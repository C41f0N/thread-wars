#include "lib/models.h"
#include "lib/vector_ops.h"
#include "raylib.h"
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PLAYERS_NUM 3

bool quitting = false;

int frameCount = 0;
pthread_mutex_t frameCountMutex;

/*
    Stores keymaps for each player
    Sequence: Up, Left, Down, Right
*/
int controls[3][4] = {{KEY_W, KEY_A, KEY_S, KEY_D},
                      {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT},
                      {KEY_T, KEY_F, KEY_G, KEY_H}};

Player *initializePlayers() {
  Color playerColors[] = {YELLOW, BLUE, GREEN, PINK};

  Player *players = (Player *)calloc(sizeof(Player), PLAYERS_NUM);

  for (int i = 0; i < PLAYERS_NUM; i++) {
    players[i].size = 50;
    players[i].speed = 10;
    // players[i].speed = 10;
    players[i].color = playerColors[i % PLAYERS_NUM];
    players[i].position = (Vector2){0 + i * (20 + players[i].size), 0};

    // Initializing mutexes
    for (int j = 0; j < PLAYERS_NUM; j++) {
      if (pthread_mutex_init(&players[j].mutex, NULL)) {
        exit(0);
      };
    }
  }

  return players;
}

Enemy *initializeEnemies(int numEnemies) {
  Enemy *enemies = calloc(numEnemies, sizeof(Enemy));

  int range = (int)(GetScreenWidth() / PLAYERS_NUM);

  for (int i = 0; i < numEnemies; i++) {
    enemies[i].size = 30;
    enemies[i].color = RED;
    enemies[i].speed = 1 + (int)((rand() % 50) / 10);
    enemies[i].position = (Vector2){((rand() % range) - (float)range / 2),
                                    (rand() % range) - (float)range / 2};
  }

  return enemies;
}

// Function to create and initialize viewports
Viewport *initializeViewports(int numPlayers, Player *players) {

  // Allocating in memory
  RenderTexture2D *renderTextures = calloc(sizeof(RenderTexture2D), numPlayers);
  Camera2D *cameras = calloc(sizeof(Camera2D), numPlayers);
  Viewport *viewports = calloc(numPlayers, sizeof(Viewport));

  // Initializing semaphores, setting the first one as active
  sem_init(&viewports[0].inputSemaphore, 0, 1);

  // Initializing the rest as inactive
  for (int i = 1; i < numPlayers; i++) {
    sem_init(&viewports[i].inputSemaphore, 0, 0);
  }

  for (int i = 0; i < numPlayers; i++) {
    // Creating render textures
    renderTextures[i] =
        LoadRenderTexture(GetScreenWidth() / numPlayers, GetScreenHeight());

    // Setting up cameras
    cameras[i].zoom = 1.0;
    cameras[i].offset = (Vector2){(int)(renderTextures[i].texture.width / 2),
                                  (int)(renderTextures[i].texture.height / 2)};

    // Setting a render texture to viewport
    viewports[i].renderTexture = &renderTextures[i];

    // Setting a camera to viewport
    viewports[i].camera = &cameras[i];

    // Setting a player to viewport
    viewports[i].player = &players[i];
  }

  return viewports;
}

void drawEnemies(Enemy *enemies, int numEnemies) {
  // Drawing all enemies
  for (int i = 0; i < numEnemies; i++) {
    DrawRectangleRounded((Rectangle){enemies[i].position.x,
                                     enemies[i].position.y, enemies[i].size,
                                     enemies[i].size},
                         0.5, 1, enemies[i].color);
  }
}

void updateEnemies(Enemy *enemies, int numEnemies, Player *players,
                   int numPlayers) {
  for (int i = 0; i < numEnemies; i++) {
    Player *closestPlayer = NULL;
    float shortestDistance = INT_MAX;

    for (int j = 0; j < numPlayers; j++) {
      pthread_mutex_lock(&players[j].mutex);

      float distance =
          getDistanceBetweenVectors(enemies[i].position, players[j].position);

      if (distance < shortestDistance) {
        shortestDistance = distance;
        closestPlayer = &players[j];
      }
      pthread_mutex_unlock(&players[j].mutex);
    };

    if (shortestDistance < (float)closestPlayer->size / 2) {
      continue;
    }

    // Getting direction to the closest player
    Vector2 direction =
        getDirectionVector2s(enemies[i].position, closestPlayer->position);

    Vector2 velocity = (Vector2){direction.x * enemies[i].speed,
                                 direction.y * enemies[i].speed};

    enemies[i].position.x += velocity.x;
    enemies[i].position.y += velocity.y;

    // Check for collision with other enemies
    bool colliding = false;
    for (int j = 0; j < numEnemies; j++) {

      if (i != j && CheckCollisionCircles(
                        enemies[i].position, (float)enemies[i].size / 2 + 1,
                        enemies[j].position, (float)enemies[j].size / 2 + 1)) {
        // If collission is detected, push the current enemy in the opposite
        // direction as the player
        colliding = true;
        direction =
            getDirectionVector2s(enemies[j].position, enemies[i].position);

        enemies[i].position.x += direction.x * (enemies[i].speed + 1);
        enemies[i].position.y += direction.y * (enemies[i].speed + 1);
      }
    }
    // Undo the move made earlier if colliding
    if (colliding) {
      enemies[i].position.x -= velocity.x;
      enemies[i].position.y -= velocity.y;
    }
  }
}

void *handleInput(void *arg) {

  ViewportThreadArgument *arguments = (ViewportThreadArgument *)arg;
  Viewport *viewports = arguments->viewports;
  int viewportIndex = arguments->viewportIndex;
  int localFrameCount = -1;

  while (true) { // Wait for current semaphore to process
    sem_wait(&viewports[viewportIndex].inputSemaphore);

    pthread_mutex_lock(&frameCountMutex);
    int x = frameCount;
    pthread_mutex_unlock(&frameCountMutex);

    if (localFrameCount == x) {
      sem_post(&viewports[(viewportIndex + 1) % PLAYERS_NUM].inputSemaphore);
    } else {
      localFrameCount = x;

      // Quit and let others quit (by giving them control) if quitting
      if (quitting) {
        sem_post(&viewports[(viewportIndex + 1) % PLAYERS_NUM].inputSemaphore);
      }

      Vector2 direction = {0, 0};

      // Up
      if (IsKeyDown(controls[viewportIndex % (int)(sizeof(controls) / 4)][0])) {
        direction.y = -1;
      }

      // Left
      if (IsKeyDown(controls[viewportIndex % (int)(sizeof(controls) / 4)][1])) {
        direction.x = -1;
      }

      // Down
      if (IsKeyDown(controls[viewportIndex % (int)(sizeof(controls) / 4)][2])) {
        direction.y = 1;
      }

      // Right
      if (IsKeyDown(controls[viewportIndex % (int)(sizeof(controls) / 4)][3])) {
        direction.x = 1;
      }

      // Zoom In
      if (IsKeyPressed(KEY_EQUAL)) {
        viewports[viewportIndex].camera->zoom += 0.25;
      }

      // Zoom Out
      if (IsKeyPressed(KEY_MINUS)) {
        viewports[viewportIndex].camera->zoom -= 0.25;
      }

      // Calculating unit vector (direction) of movement
      float magnitude =
          sqrtf(direction.x * direction.x + direction.y * direction.y);
      if (magnitude != 0) {
        direction = (Vector2){direction.x / magnitude, direction.y / magnitude};
      }

      // Multiplying direction with speed to get velocity
      Vector2 velocity = {direction.x * viewports[viewportIndex].player->speed,
                          direction.y * viewports[viewportIndex].player->speed};

      // Moving the player via velocity
      pthread_mutex_lock(&viewports[viewportIndex].player->mutex);
      viewports[viewportIndex].player->position.x += velocity.x;
      viewports[viewportIndex].player->position.y += velocity.y;
      pthread_mutex_unlock(&viewports[viewportIndex].player->mutex);

      sem_post(&viewports[(viewportIndex + 1) % PLAYERS_NUM].inputSemaphore);
    }
  }
}

typedef struct {
  int centerX, centerY, radius;
  Color color;
} Circle;

// Creates randomly placed/sized Circles
Circle *initCircles(int circleNum, int bounds) {

  Circle *circles = calloc(circleNum, sizeof(Circle));

  for (int i = 0; i < circleNum; i++) {
    circles[i].centerX = (rand() % (2 * GetScreenWidth())) - GetScreenWidth();
    circles[i].centerY = (rand() % (2 * GetScreenHeight())) - GetScreenHeight();
    circles[i].radius = rand() % 5;
    circles[i].color = WHITE;
  }

  return circles;
}

void drawCircles(Circle *circles, int circleNum) {
  for (int i = 0; i < circleNum; i++) {
    DrawCircle(circles[i].centerX, circles[i].centerY, circles[i].radius,
               circles[i].color);
  }
}

void drawPlayers(Player *players, int numPlayers) {
  for (int i = 0; i < PLAYERS_NUM; i++) {
    pthread_mutex_lock(&players[i].mutex);
    DrawRectangleRounded((Rectangle){players[i].position.x,
                                     players[i].position.y, players[i].size,
                                     players[i].size},
                         0.5, 1, players[i].color);
    pthread_mutex_unlock(&players[i].mutex);
  }
}

void draw(Viewport *viewports, Player *players, Circle *circles, int circleNum,
          Enemy *enemies, int numEnemies) {

  // Drawing on every viewport
  for (int i = 0; i < PLAYERS_NUM; i++) {

    // Updating camera to follow player
    pthread_mutex_lock(&players[i].mutex);
    viewports[i].camera->target =
        (Vector2){(int)(players[i].position.x), (int)(players[i].position.y)};
    pthread_mutex_unlock(&players[i].mutex);

    // Setting to draw on the viewport's RenderTexture
    BeginTextureMode(*viewports[i].renderTexture);

    // Drawing with respect to the viewport's Camera
    BeginMode2D(*viewports[i].camera);

    // Start from a clean slate
    ClearBackground(BLACK);

    // Random shit because I needed a background
    drawCircles(circles, circleNum);

    // Draw all enemies
    drawEnemies(enemies, numEnemies);

    // Drawing all players
    drawPlayers(players, PLAYERS_NUM);

    EndMode2D();
    EndTextureMode();
  }

  // BeginDrawing();
  ClearBackground(BLACK);

  // Drawing the prepared viewports to a single screen sized rectangle
  for (int i = 0; i < PLAYERS_NUM; i++) {
    DrawTextureRec(viewports[i].renderTexture->texture,
                   (Rectangle){0, 0, viewports[i].renderTexture->texture.width,
                               -viewports[i].renderTexture->texture.height},
                   (Vector2){i * (int)(GetScreenWidth() / PLAYERS_NUM), 0},
                   WHITE);
  }

  // Drawing line(s) between screens
  for (int i = 1; i < PLAYERS_NUM; i++) {
    DrawRectangle(i * (int)(GetScreenWidth() / PLAYERS_NUM) - 2, 0, 4,
                  GetScreenHeight(), WHITE);
  }

  DrawFPS(0, 0);

  // EndDrawing();
}

void killViewports(Viewport *viewports) {
  quitting = true;
  for (int i = 0; i < PLAYERS_NUM; i++) {
    // Join all threads
    // pthread_join(viewports[i].thread, NULL);

    // Unloading all render textures out of the GPU
    UnloadRenderTexture(*viewports[i].renderTexture);

    // Destroy semaphores
    sem_destroy(&viewports[i].inputSemaphore);
  }
}

int main(int argc, char **args) {

  srand(time(NULL));
  InitWindow(0, 0, "Thread Wars");

  if (!IsWindowFullscreen()) {
    ToggleFullscreen();
  }

  int numEnemies = 1000;

  Player *players = initializePlayers();
  Enemy *enemies = initializeEnemies(numEnemies);
  Viewport *viewports = initializeViewports(PLAYERS_NUM, players);

  pthread_mutex_init(&frameCountMutex, NULL);

  SetTargetFPS(60);

  int circleNum = 30;
  Circle *circles = initCircles(circleNum, 3);

  bool paused = false;

  // Initializing viewport threads

  // Creating viewport threads
  for (int i = 0; i < PLAYERS_NUM; i++) {
    ViewportThreadArgument *args = malloc(sizeof(ViewportThreadArgument));
    args->viewportIndex = i;
    args->viewports = viewports;

    pthread_create(&viewports[i].thread, NULL, handleInput, (void *)args);
  }

  while (!WindowShouldClose()) {

    // Pausing
    if (IsKeyPressed(KEY_P)) {
      paused = !paused;
    }

    // Update
    if (!paused) {
      updateEnemies(enemies, numEnemies, players, PLAYERS_NUM);
    }

    BeginDrawing();
    // Drawing everything to screen
    draw(viewports, players, circles, circleNum, enemies, numEnemies);

    if (paused) {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenWidth(), Fade(BLACK, 0.7));
      char text[] = "PAUSED";
      int fontSize = GetScreenHeight() * 0.05;
      DrawText(text, GetScreenWidth() / 2 - MeasureText(text, fontSize) / 2,
               GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
    }

    EndDrawing();
    pthread_mutex_lock(&frameCountMutex);
    frameCount++;
    pthread_mutex_unlock(&frameCountMutex);
  }

  killViewports(viewports);

  CloseWindow();

  return 0;
}