#include "lib/models.h"
#include "lib/vector_ops.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PLAYERS_NUM 2

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
    players[i].speed = 6;
    players[i].color = playerColors[i % PLAYERS_NUM];
    players[i].position = (Vector2){0 + i * (20 + players[i].size), 0};
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
    Player *closestPlayer = &players[0];
    float shortestDistance =
        getDistanceBetweenVectors(enemies[i].position, closestPlayer->position);

    for (int j = 1; j < numPlayers; j++) {
      float distance =
          getDistanceBetweenVectors(enemies[i].position, players[j].position);

      if (distance < shortestDistance) {
        shortestDistance = distance;
        closestPlayer = &players[j];
      }
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

void handleInput(Viewport *viewport) {

  for (int i = 0; i < PLAYERS_NUM; i++) {
    Vector2 direction = {0, 0};

    // Up
    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][0])) {
      direction.y = -1;
    }

    // Left
    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][1])) {
      direction.x = -1;
    }

    // Down
    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][2])) {
      direction.y = 1;
    }

    // Right
    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][3])) {
      direction.x = 1;
    }

    // Zoom In
    if (IsKeyPressed(KEY_EQUAL)) {
      viewport[i].camera->zoom += 0.25;
    }

    // Zoom Out
    if (IsKeyPressed(KEY_MINUS)) {
      viewport[i].camera->zoom -= 0.25;
    }

    // Calculating unit vector (direction) of movement
    float magnitude =
        sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (magnitude != 0) {
      direction = (Vector2){direction.x / magnitude, direction.y / magnitude};
    }

    // Multiplying direction with speed to get velocity
    Vector2 velocity = {direction.x * viewport[i].player->speed,
                        direction.y * viewport[i].player->speed};

    // Moving the player via velocity
    viewport[i].player->position.x += velocity.x;
    viewport[i].player->position.y += velocity.y;
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

void draw(Viewport *viewports, Player *players, Circle *circles, int circleNum,
          Enemy *enemies, int numEnemies) {

  // Drawing on every viewport
  for (int i = 0; i < PLAYERS_NUM; i++) {

    // Updating camera to follow player
    viewports[i].camera->target =
        (Vector2){(int)(players[i].position.x), (int)(players[i].position.y)};

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
    for (int j = 0; j < PLAYERS_NUM; j++) {
      // DrawRectangle(players[j].pos_x, players[j].pos_y, players[j].size,
      //               players[j].size, players[j].color);
      DrawRectangleRounded((Rectangle){players[j].position.x,
                                       players[j].position.y, players[j].size,
                                       players[j].size},
                           0.5, 1, players[j].color);
    }

    EndMode2D();
    EndTextureMode();
  }

  BeginDrawing();
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

  EndDrawing();
}

void killViewports(Viewport *viewports) {
  // Unloading all render textures out of the GPU
  for (int i = 0; i < PLAYERS_NUM; i++) {
    UnloadRenderTexture(*viewports[i].renderTexture);
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

  SetTargetFPS(60);

  int circleNum = 30;
  Circle *circles = initCircles(circleNum, 3);

  bool paused = false;

  while (!WindowShouldClose()) {

    // Pausing
    if (IsKeyPressed(KEY_P)) {
      paused = !paused;
    }
    // Handling Player Input (Should be done as threads later on)
    if (!paused) {
      handleInput(viewports);
      updateEnemies(enemies, numEnemies, players, PLAYERS_NUM);
    }
    // Drawing everything to screen
    draw(viewports, players, circles, circleNum, enemies, numEnemies);
  }

  killViewports(viewports);

  CloseWindow();

  return 0;
}