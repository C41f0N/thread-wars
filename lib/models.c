#include <pthread.h>
#include <raylib.h>
#include <semaphore.h>
#include <stdlib.h>

// Player Struct
typedef struct {
  Vector2 position;
  float speed;
  int size;
  int health;
  Color color;
  pthread_mutex_t mutex;
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
  sem_t inputSemaphore;
  pthread_t thread;
} Viewport;

// Viewport Thread Arguments Struct
typedef struct {
  Viewport *viewports;
  int viewportIndex;
} ViewportThreadArgument;
