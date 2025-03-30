#include <pthread.h>
#include <raylib.h>
#include <semaphore.h>
#include <stdlib.h>

// Player Struct
typedef struct {
  Vector2 position;
  float speed;
  int size;
  float health;
  Color color;
  pthread_mutex_t mutex;
} Player;

// Enemy Struct
typedef struct {
  Vector2 position;
  float damage;
  int speed;
  int size;
  bool active;
  Color color;
} Enemy;

// Viewport Struct
typedef struct {
  Camera2D *camera;
  RenderTexture2D *renderTexture;
  Player *player;
  sem_t inputSemaphore;
  pthread_t thread;
} Viewport;

// Game Struct
typedef struct {
  bool paused;
  bool isQuitting;
  int targetFPS;

  int frameCount;
  pthread_mutex_t frameCountMutex;

  Player *players;
  int playerCount;

  Enemy *enemies;
  int maxEnemies;

  int enemyCount;
  pthread_mutex_t enemyCountMutex;

  Viewport *viewports;
} Game;

// Viewport Thread Arguments Struct
typedef struct {
  Game *game;
  int viewportIndex;
} ViewportThreadArgument;
