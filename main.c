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

/*
    Stores keymaps for each player
    Sequence: Up, Left, Down, Right
*/
int controls[3][5] = {{KEY_W, KEY_A, KEY_S, KEY_D, KEY_SPACE},
                      {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_ENTER},
                      {KEY_T, KEY_F, KEY_G, KEY_H, KEY_U}};

void initializePlayers(Game *game) {
  Color playerColors[] = {YELLOW, BLUE, GREEN, PINK};

  game->players = (Player *)calloc(sizeof(Player), game->playerCount);

  for (int i = 0; i < game->playerCount; i++) {
    game->players[i].size = 50;
    game->players[i].speed = (float)600 / game->targetFPS;
    game->players[i].health = 100;
    game->players[i].color = playerColors[i % game->playerCount];
    game->players[i].position =
        (Vector2){0 + i * (20 + game->players[i].size), 0};

    // Initializing mutexes
    for (int j = 0; j < game->playerCount; j++) {
      if (pthread_mutex_init(&game->players[j].mutex, NULL)) {
        exit(0);
      };
    }
  }
}

int getClosestEnemyIndex(Game *game, Vector2 from) {
  float shortestDistance = INT_MAX;
  int closestEnemyIndex = -1;

  for (int i = 0; i < game->maxEnemies; i++) {
    if (game->enemies[i].active) {

      float distance =
          getDistanceBetweenVectors(game->enemies[i].position, from);

      if (distance < shortestDistance) {
        shortestDistance = distance;
        closestEnemyIndex = i;
      }
    }
  };

  return closestEnemyIndex;
}

void initializeEnemies(Game *game) {
  game->enemies = calloc(game->maxEnemies, sizeof(Enemy));

  int range = (int)(GetScreenWidth() / game->playerCount);

  for (int i = 0; i < game->maxEnemies; i++) {
    game->enemies[i].size = 30;
    game->enemies[i].color = RED;
    game->enemies[i].active = i < game->enemyCount;
    game->enemies[i].damage = 5;
    game->enemies[i].speed = (100 + (rand() % 101)) / game->targetFPS;
    game->enemies[i].position = (Vector2){((rand() % range) - (float)range / 2),
                                          (rand() % range) - (float)range / 2};
  }

  // Activate fixed number of enemies
}

// Function to create and initialize viewports
void initializeViewports(Game *game) {

  // Allocating in memory
  RenderTexture2D *renderTextures =
      calloc(sizeof(RenderTexture2D), game->playerCount);
  Camera2D *cameras = calloc(sizeof(Camera2D), game->playerCount);
  game->viewports = calloc(game->playerCount, sizeof(Viewport));

  // Initializing semaphores, setting the first one as active
  sem_init(&game->viewports[0].inputSemaphore, 0, 1);

  // Initializing the rest as inactive
  for (int i = 1; i < game->playerCount; i++) {
    sem_init(&game->viewports[i].inputSemaphore, 0, 0);
  }

  for (int i = 0; i < game->playerCount; i++) {
    // Creating render textures
    renderTextures[i] = LoadRenderTexture(GetScreenWidth() / game->playerCount,
                                          GetScreenHeight());

    // Setting up cameras
    cameras[i].zoom = 1.0;
    cameras[i].offset = (Vector2){(int)(renderTextures[i].texture.width / 2),
                                  (int)(renderTextures[i].texture.height / 2)};

    // Setting a render texture to viewport
    game->viewports[i].renderTexture = &renderTextures[i];

    // Setting a camera to viewport
    game->viewports[i].camera = &cameras[i];

    // Setting a player to viewport
    game->viewports[i].player = &game->players[i];
  }
}

void drawEnemies(Game *game) {
  // Drawing all enemies
  for (int i = 0; i < game->maxEnemies; i++) {
    if (game->enemies[i].active) {
      DrawRectangleRounded(
          (Rectangle){
              game->enemies[i].position.x + (float)game->enemies[i].size / 2,
              game->enemies[i].position.y + (float)game->enemies[i].size / 2,
              game->enemies[i].size, game->enemies[i].size},
          0.5, 1, game->enemies[i].color);
    }
  }
}

void updateEnemies(Game *game) {
  for (int i = 0; i < game->maxEnemies; i++) {
    // Skip inactive enemies
    if (!game->enemies[i].active)
      continue;

    Player *closestPlayer = NULL;
    float shortestDistance = INT_MAX;

    for (int j = 0; j < game->playerCount; j++) {
      pthread_mutex_lock(&game->players[j].mutex);

      float distance = getDistanceBetweenVectors(game->enemies[i].position,
                                                 game->players[j].position);

      if (distance < shortestDistance) {
        shortestDistance = distance;
        closestPlayer = &game->players[j];
      }
      pthread_mutex_unlock(&game->players[j].mutex);
    };

    // if close enough to player, stop and give him damage
    if (shortestDistance < (float)closestPlayer->size) {
      pthread_mutex_lock(&closestPlayer->mutex);
      closestPlayer->health -= (float)game->enemies[i].damage / game->targetFPS;
      pthread_mutex_unlock(&closestPlayer->mutex);

      continue;
    }

    // Getting direction to the closest player
    Vector2 direction = getDirectionVector2s(game->enemies[i].position,
                                             closestPlayer->position);

    Vector2 velocity = (Vector2){direction.x * game->enemies[i].speed,
                                 direction.y * game->enemies[i].speed};

    game->enemies[i].position.x += velocity.x;
    game->enemies[i].position.y += velocity.y;

    // Check for collision with other enemies
    bool colliding = false;
    for (int j = 0; j < game->maxEnemies; j++) {

      // Skip inactive enemies
      if (!game->enemies[j].active)
        continue;

      if (i != j &&
          CheckCollisionCircles(
              game->enemies[i].position, (float)game->enemies[i].size * 0.55,
              game->enemies[j].position, (float)game->enemies[j].size * 0.55)) {
        // If collission is detected, push the current enemy in the opposite
        // direction as the player
        colliding = true;
        direction = getDirectionVector2s(game->enemies[j].position,
                                         game->enemies[i].position);
        // direction = normalizeVector2(
        //     (Vector2){(float)(rand() % 20 - 10), (float)(rand() % 20 - 10)});

        game->enemies[i].position.x +=
            direction.x * (game->enemies[i].speed * 0.5);
        game->enemies[i].position.y +=
            direction.y * (game->enemies[i].speed * 0.5);
      }
    }
    // Undo the move made earlier if colliding
    if (colliding) {
      game->enemies[i].position.x -= velocity.x;
      game->enemies[i].position.y -= velocity.y;
    }
  }
}

void addEnemies(Game *game, int n) {
  int i = 0;
  pthread_mutex_lock(&game->enemyCountMutex);
  while (n > 0 && game->enemyCount <= game->maxEnemies &&
         i < game->maxEnemies) {

    if (!game->enemies[i].active) {
      game->enemies[i].active = true;
      game->enemies[i].size = 30;
      game->enemies[i].color = RED;
      game->enemies[i].damage = 5;
      game->enemies[i].speed = 200 / game->targetFPS;
      game->enemies[i].position = (Vector2){
          ((rand() % GetScreenWidth()) - (float)GetScreenWidth() / 2),
          (rand() % GetScreenHeight()) - (float)GetScreenHeight() / 2};
      game->enemyCount++;
      n--;
    }

    i++;
  }

  pthread_mutex_unlock(&game->enemyCountMutex);
}

void killEnemy(Game *game, int enemyIndex) {
  game->enemies[enemyIndex].active = false;
  pthread_mutex_lock(&game->enemyCountMutex);
  game->enemyCount--;
  pthread_mutex_unlock(&game->enemyCountMutex);
}

void *updatePlayer(void *arg) {

  ViewportThreadArgument *arguments = (ViewportThreadArgument *)arg;
  Game *game = arguments->game;
  int viewportIndex = arguments->viewportIndex;
  int localFrameCount = -1;
  Viewport *viewports = game->viewports;

  while (true) { // Wait for current semaphore to process
    sem_wait(&viewports[viewportIndex].inputSemaphore);

    pthread_mutex_lock(&game->frameCountMutex);
    int x = game->frameCount;
    pthread_mutex_unlock(&game->frameCountMutex);

    if (localFrameCount == x) {
      sem_post(
          &viewports[(viewportIndex + 1) % game->playerCount].inputSemaphore);
    } else {
      localFrameCount = x;

      // Quit and let others quit (by giving them control) if quitting
      if (game->isQuitting) {
        sem_post(
            &viewports[(viewportIndex + 1) % game->playerCount].inputSemaphore);
        break;
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

      // Shoot
      if (IsKeyPressed(
              controls[viewportIndex % (int)(sizeof(controls) / 4)][4])) {
        // Get closest enemy from player
        pthread_mutex_lock(&viewports[viewportIndex].player->mutex);
        int closestEnemyIndex = getClosestEnemyIndex(
            game, viewports[viewportIndex].player->position);
        pthread_mutex_unlock(&viewports[viewportIndex].player->mutex);

        if (closestEnemyIndex != -1) {
          killEnemy(game, closestEnemyIndex);
        }
      }

      // Zoom In
      if (IsKeyPressed(KEY_EQUAL)) {
        viewports[viewportIndex].camera->zoom += 0.25;
      }

      // Zoom Out
      if (IsKeyPressed(KEY_MINUS)) {
        viewports[viewportIndex].camera->zoom -= 0.25;
      }

      // Normalizing direction of movement
      direction = normalizeVector2(direction);

      // Multiplying direction with speed to get velocity
      Vector2 velocity = {direction.x * viewports[viewportIndex].player->speed,
                          direction.y * viewports[viewportIndex].player->speed};

      if (!game->paused) {
        // Moving the player via velocity
        pthread_mutex_lock(&viewports[viewportIndex].player->mutex);
        viewports[viewportIndex].player->position.x += velocity.x;
        viewports[viewportIndex].player->position.y += velocity.y;
        pthread_mutex_unlock(&viewports[viewportIndex].player->mutex);
      }

      // Giving control to the next viewport thread
      sem_post(
          &viewports[(viewportIndex + 1) % game->playerCount].inputSemaphore);
    }
  }

  return NULL;
}

void drawPlayers(Game *game) {
  for (int i = 0; i < game->playerCount; i++) {
    pthread_mutex_lock(&game->players[i].mutex);
    DrawRectangleRounded(
        (Rectangle){game->players[i].position.x, game->players[i].position.y,
                    game->players[i].size, game->players[i].size},
        0.5, 1, game->players[i].color);
    pthread_mutex_unlock(&game->players[i].mutex);
  }
}

void draw(Game *game) {

  // Drawing on every viewport
  for (int i = 0; i < game->playerCount; i++) {

    // Updating camera to follow player
    pthread_mutex_lock(&game->players[i].mutex);
    game->viewports[i].camera->target = (Vector2){
        (int)(game->players[i].position.x), (int)(game->players[i].position.y)};
    pthread_mutex_unlock(&game->players[i].mutex);

    // Setting to draw on the viewport's RenderTexture
    BeginTextureMode(*game->viewports[i].renderTexture);

    // Drawing with respect to the viewport's Camera
    BeginMode2D(*game->viewports[i].camera);

    // Start from a clean slate
    ClearBackground(BLACK);

    // Draw all enemies
    drawEnemies(game);

    // Drawing all players
    drawPlayers(game);

    // Draw player health
    char playerHealthText[128];
    int playerHealthFontSize = 18;
    pthread_mutex_lock(&game->players[i].mutex);
    sprintf(playerHealthText, "Player %d Health: %.1f", i,
            game->players[i].health);
    pthread_mutex_unlock(&game->players[i].mutex);

    EndMode2D();

    DrawText(playerHealthText,
             game->viewports[i].renderTexture->texture.width / 2 -
                 MeasureText(playerHealthText, playerHealthFontSize) / 2,
             game->viewports[i].renderTexture->texture.height -
                 playerHealthFontSize -
                 game->viewports[i].renderTexture->texture.height * 0.01,
             playerHealthFontSize, WHITE);

    EndTextureMode();
  }

  ClearBackground(BLACK);

  // Drawing the prepared viewports to a single screen sized rectangle
  for (int i = 0; i < game->playerCount; i++) {
    DrawTextureRec(
        game->viewports[i].renderTexture->texture,
        (Rectangle){0, 0, game->viewports[i].renderTexture->texture.width,
                    -game->viewports[i].renderTexture->texture.height},
        (Vector2){i * (int)(GetScreenWidth() / game->playerCount), 0}, WHITE);
  }

  // Drawing line(s) between screens
  for (int i = 1; i < game->playerCount; i++) {
    DrawRectangle(i * (int)(GetScreenWidth() / game->playerCount) - 2, 0, 4,
                  GetScreenHeight(), WHITE);
  }

  // Draw FPS
  DrawFPS(0, 0);

  // Draw enemies left
  pthread_mutex_lock(&game->enemyCountMutex);
  int enemiesLeft = game->enemyCount;
  pthread_mutex_unlock(&game->enemyCountMutex);

  char enemiesAliveText[128];
  sprintf(enemiesAliveText, "Enemies Alive: %d", enemiesLeft);
  int fontSize = 25;

  DrawText(enemiesAliveText,
           GetScreenWidth() / 2 - MeasureText(enemiesAliveText, fontSize) / 2,
           GetScreenHeight() * 0.01, 25, WHITE);
}

void killViewports(Game *game) {
  game->isQuitting = true;
  for (int i = 0; i < game->playerCount; i++) {
    // Join all threads
    pthread_join(game->viewports[i].thread, NULL);

    // Unloading all render textures out of the GPU
    UnloadRenderTexture(*game->viewports[i].renderTexture);

    // Destroy semaphores
    sem_destroy(&game->viewports[i].inputSemaphore);
  }
}

int main(int argc, char **args) {

  srand(time(NULL));
  InitWindow(0, 0, "Thread Wars");

  if (!IsWindowFullscreen()) {
    ToggleFullscreen();
  }

  Game game;

  game.playerCount = 2;
  game.maxEnemies = 100;
  game.enemyCount = 50;
  game.targetFPS = 60;
  game.paused = false;

  initializePlayers(&game);
  initializeEnemies(&game);
  initializeViewports(&game);

  pthread_mutex_init(&game.frameCountMutex, NULL);
  pthread_mutex_init(&game.enemyCountMutex, NULL);

  SetTargetFPS(game.targetFPS);

  // Creating viewport threads
  for (int i = 0; i < game.playerCount; i++) {
    ViewportThreadArgument *args = malloc(sizeof(ViewportThreadArgument));
    args->viewportIndex = i;
    args->game = &game;

    pthread_create(&game.viewports[i].thread, NULL, updatePlayer, (void *)args);
  }

  while (!WindowShouldClose()) {

    // Pausing
    if (IsKeyPressed(KEY_P)) {
      game.paused = !game.paused;
    }

    // Adding enemies
    if (IsKeyPressed(KEY_BACKSPACE)) {
      addEnemies(&game, 5);
    }

    // Update
    if (!game.paused) {
      updateEnemies(&game);
    }

    BeginDrawing();
    // Drawing everything to screen
    draw(&game);

    if (game.paused) {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenWidth(), Fade(BLACK, 0.7));
      char enemiesAliveText[] = "PAUSED";
      int fontSize = GetScreenHeight() * 0.05;
      DrawText(enemiesAliveText,
               GetScreenWidth() / 2 -
                   MeasureText(enemiesAliveText, fontSize) / 2,
               GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
    }

    EndDrawing();
    pthread_mutex_lock(&game.frameCountMutex);
    game.frameCount++;
    pthread_mutex_unlock(&game.frameCountMutex);
  }

  killViewports(&game);

  CloseWindow();

  return 0;
}