#include "raylib.h"
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
    players[i].pos_x = 0;
    players[i].pos_y = 0;
    players[i].size = 50;
    players[i].color = colors[i % PLAYERS_NUM];
  }

  return players;
}

void handleInput(Player *players, int speed) {
  for (int i = 0; i < PLAYERS_NUM; i++) {

    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][0])) {
      players[i].pos_y -= speed;
    }

    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][1])) {
      players[i].pos_x -= speed;
    }

    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][2])) {
      players[i].pos_y += speed;
    }

    if (IsKeyDown(controls[i % (int)(sizeof(controls) / 4)][3])) {
      players[i].pos_x += speed;
    }
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
                                  (int)(renderTextures[i].texture.width / 2)};
  }

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    handleInput(players, 4);

    for (int i = 0; i < PLAYERS_NUM; i++) {
      BeginTextureMode(renderTextures[i]);

      cameras[i].target =
          (Vector2){(int)(players[i].pos_x - players[i].size / 2),
                    (int)(players[i].pos_y - players[i].size / 2)};

      BeginMode2D(cameras[i]);

      ClearBackground(WHITE);
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
    EndDrawing();
  }

  for (int i = 0; i < PLAYERS_NUM; i++) {
    UnloadRenderTexture(renderTextures[i]);
  }

  CloseWindow();

  return 0;
}