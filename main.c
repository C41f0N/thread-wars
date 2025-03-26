#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define PLAYERS_NUM 2

typedef struct {
  int pos_x, pos_y;
  int size;
  Color color;
} Player;

Color colors[] = {RED, YELLOW, BLUE, GREEN};

/*
    Stores keymaps for each player
    Sequence: Up, Left, Down, Right
*/
int controls[3][4] = {{KEY_W, KEY_A, KEY_S, KEY_D},
                      {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT},
                      {KEY_T, KEY_F, KEY_G, KEY_H}};

Player *initializePlayers() {
  Player *players = (Player *)calloc(sizeof(Player), PLAYERS_NUM);

  for (int i = 0; i < PLAYERS_NUM; i++) {
    players[i].size = 50;
    players[i].color = colors[i % PLAYERS_NUM];
    players[i].pos_x = 0 + i * (20 + players[i].size);
    players[i].pos_y = 0;
  }

  return players;
}

void handleInput(Player *players, int speed) {

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

    // Normalizing direction
    float magnitude =
        sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (magnitude != 0) {
      direction = (Vector2){direction.x / magnitude, direction.y / magnitude};
    }

    Vector2 velocity = {direction.x * speed, direction.y * speed};

    players[i].pos_x += velocity.x;
    players[i].pos_y += velocity.y;
  }
}

typedef struct {
  int centerX, centerY, radius;
  Color color;
} Circle;

Circle *initCircles(int n, int bounds) {

  Circle *circles = calloc(n, sizeof(Circle));

  for (int i = 0; i < n; i++) {
    circles[i].centerX = (rand() % (2 * GetScreenWidth())) - GetScreenWidth();
    circles[i].centerY = (rand() % (2 * GetScreenHeight())) - GetScreenHeight();
    circles[i].radius = rand() % 5;
    circles[i].color = WHITE;
  }

  return circles;
}

void drawCircles(Circle *circles, int n) {
  for (int i = 0; i < n; i++) {
    DrawCircle(circles[i].centerX, circles[i].centerY, circles[i].radius,
               circles[i].color);
  }
}

int main(int argc, char **args) {

  InitWindow(0, 0, "Thread Wars");

  if (!IsWindowFullscreen()) {
    ToggleFullscreen();
  }

  Player *players = initializePlayers();

  // Init render textures
  RenderTexture2D *renderTextures =
      calloc(sizeof(RenderTexture2D), PLAYERS_NUM);

  for (int i = 0; i < PLAYERS_NUM; i++) {
    renderTextures[i] =
        LoadRenderTexture(GetScreenWidth() / PLAYERS_NUM, GetScreenHeight());
  }

  // init cameras
  Camera2D *cameras = calloc(sizeof(Camera2D), PLAYERS_NUM);

  for (int i = 0; i < PLAYERS_NUM; i++) {
    cameras[i].zoom = 1.0;
    cameras[i].offset = (Vector2){(int)(renderTextures[i].texture.width / 2),
                                  (int)(renderTextures[i].texture.height / 2)};
  }

  int n = 2000;
  Circle *circles = initCircles(n, 2);

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    handleInput(players, 4);

    for (int i = 0; i < PLAYERS_NUM; i++) {
      BeginTextureMode(renderTextures[i]);

      cameras[i].target =
          (Vector2){(int)(players[i].pos_x), (int)(players[i].pos_y)};
      ClearBackground(BLACK);

      BeginMode2D(cameras[i]);
      drawCircles(circles, n);

      for (int j = 0; j < PLAYERS_NUM; j++) {
        DrawRectangle(players[j].pos_x, players[j].pos_y, players[j].size,
                      players[j].size, players[j].color);
      }

      EndMode2D();
      EndTextureMode();
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 0; i < PLAYERS_NUM; i++) {
      DrawTextureRec(renderTextures[i].texture,
                     (Rectangle){0, 0, renderTextures[i].texture.width,
                                 -renderTextures[i].texture.height},
                     (Vector2){i * (int)(GetScreenWidth() / PLAYERS_NUM), 0},
                     WHITE);
    }

    // Drawing line between screens
    for (int i = 1; i < PLAYERS_NUM; i++) {
      DrawRectangle(i * (int)(GetScreenWidth() / PLAYERS_NUM) - 2, 0, 4,
                    GetScreenHeight(), WHITE);
    }

    DrawFPS(0, 0);

    EndDrawing();
  }

  for (int i = 0; i < PLAYERS_NUM; i++) {
    UnloadRenderTexture(renderTextures[i]);
  }

  CloseWindow();

  return 0;
}