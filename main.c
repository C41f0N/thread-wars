#include "raylib.h"
#include <stdio.h>

int main(int argc, char **args) {

  InitWindow(0, 0, "Thread Wars");

  if (!IsWindowFullscreen()) {
    ToggleFullscreen();
  }

  SetTargetFPS(60);
  char text[] = "Thread Wars";
  int fontSize = 40;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(WHITE);

    if (IsKeyDown(KEY_DOWN)) {
      fontSize--;
    }

    if (IsKeyDown(KEY_UP)) {
      fontSize++;
    }

    DrawText(text, GetScreenWidth() / 2 - MeasureText(text, fontSize) / 2,
             GetScreenHeight() / 2 - fontSize / 2, fontSize, BLACK);

    DrawFPS(0, 0);

    EndDrawing();
  }

  CloseWindow();

  return 0;
}