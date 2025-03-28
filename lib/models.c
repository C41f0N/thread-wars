#include <raylib.h>
#include <stdlib.h>

// Player Struct
typedef struct {
  Vector2 position;
  int speed;
  int size;
  int health;
  Color color;
} Player;

// Enemy Struct
typedef struct {
  Vector2 position;
  int speed;
  int size;
  int health;
  Color color;
} Enemy;

// Viewport Struct
typedef struct {
  Camera2D *camera;
  RenderTexture2D *renderTexture;
  Player *player;
} Viewport;

// Function to create and initialize viewports
Viewport *initializeViewports(int numPlayers, Player *players) {

  // Allocating in memory
  RenderTexture2D *renderTextures = calloc(sizeof(RenderTexture2D), numPlayers);
  Camera2D *cameras = calloc(sizeof(Camera2D), numPlayers);
  Viewport *viewports = calloc(numPlayers, sizeof(Viewport));

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