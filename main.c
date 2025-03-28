#include "models.c"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define PLAYERS_NUM 2

/*
    Stores keymaps for each player
    Sequence: Up, Left, Down, Right
*/
int controls[3][4] = {{KEY_W, KEY_A, KEY_S, KEY_D},
                      {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT},
                      {KEY_T, KEY_F, KEY_G, KEY_H}};

Player *initializePlayers() {
  Color playerColors[] = {RED, YELLOW, BLUE, GREEN};

  Player *players = (Player *)calloc(sizeof(Player), PLAYERS_NUM);

  for (int i = 0; i < PLAYERS_NUM; i++) {
    players[i].size = 50;
    players[i].color = playerColors[i % PLAYERS_NUM];
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

    // Calculating unit vector (direction) of movement
    float magnitude =
        sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (magnitude != 0) {
      direction = (Vector2){direction.x / magnitude, direction.y / magnitude};
    }

    // Multiplying direction with speed to get velocity
    Vector2 velocity = {direction.x * speed, direction.y * speed};

    // Moving the player via velocity
    players[i].pos_x += velocity.x;
    players[i].pos_y += velocity.y;
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

void draw(Viewport *viewports, Player *players, Circle *circles,
          int circleNum) {

  // Drawing on every viewport
  for (int i = 0; i < PLAYERS_NUM; i++) {

    // Updating camera to follow player
    viewports[i].camera->target =
        (Vector2){(int)(players[i].pos_x), (int)(players[i].pos_y)};

    // Setting to draw on the viewport's RenderTexture
    BeginTextureMode(*viewports[i].renderTexture);

    // Drawing with respect to the viewport's Camera
    BeginMode2D(*viewports[i].camera);

    // Start from a clean slate
    ClearBackground(BLACK);

    // Random shit because I needed a background
    drawCircles(circles, circleNum);

    // Drawing all players
    for (int j = 0; j < PLAYERS_NUM; j++) {
      // DrawRectangle(players[j].pos_x, players[j].pos_y, players[j].size,
      //               players[j].size, players[j].color);
      DrawRectangleRounded((Rectangle){players[j].pos_x, players[j].pos_y,
                                       players[j].size, players[j].size},
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

  InitWindow(0, 0, "Thread Wars");

  if (!IsWindowFullscreen()) {
    ToggleFullscreen();
  }

  Player *players = initializePlayers();
  Viewport *viewports = initializeViewports(PLAYERS_NUM, players);

  SetTargetFPS(60);

  int circleNum = 3000;
  Circle *circles = initCircles(circleNum, 3);

  while (!WindowShouldClose()) {
    // Handling Player Input (Should be done as threads later on)
    handleInput(players, 6);

    // Drawing everything to screen
    draw(viewports, players, circles, circleNum);
  }

  killViewports(viewports);

  CloseWindow();

  return 0;
}