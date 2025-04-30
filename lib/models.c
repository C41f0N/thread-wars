#include <pthread.h>
#include <raylib.h>
#include <semaphore.h>
#include <stdlib.h>

// Player Struct
typedef struct {
  Vector2 position;
  int flipDir;
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

// Solar Charger Struct
typedef struct {
  int width, height;
  Vector2 position;
  bool active;
} SolarCharger;

// Viewport Struct
typedef struct {
  Camera2D *camera;
  RenderTexture2D *renderTexture;
  Player *player;
  sem_t inputSemaphore;
  pthread_t thread;
} Viewport;

typedef struct {
  Vector2 position;
  int size;
  bool active;
} SolarCell;

// Wave Struct
typedef struct {
  int numEnemies; // number of enemies to spawn
  int waitTime;   // time in seconds before enemies are spawned
} EnemyWave;

// Game Struct
typedef struct {
  bool paused;
  bool isQuitting;
  int targetFPS;
  int mapSize;
  bool gameOver, gameWon;

  int numWaves;
  EnemyWave *waves;

  int lastWaveFrame, currentWave;

  int frameCount;

  float battery;
  pthread_mutex_t batteryMutex;

  Player *players;
  int playerCount;

  Enemy *enemies;
  int maxEnemies;

  int solarCellsCollected;
  int maxSolarCells;
  SolarCell *solarCells;
  pthread_mutex_t solarCellsMutex;

  SolarCharger *solarChargers;
  pthread_t *solarChargesComputingThreads;
  int maxSolarChargers, numSolarChargesComputers;

  int enemyCount;
  pthread_mutex_t enemyCountMutex;

  char message[256];
  float messageOpacity;
  int messageDuration, messageAddedFrame, messageFontSize;

  Viewport *viewports;
  Texture2D playerTextures[2]; // Add this for player textures
} Game;

// Viewport Thread Arguments Struct
typedef struct {
  Game *game;
  int viewportIndex;
} ViewportThreadArgument;

// Solar Charger Thread Arguments Struct
typedef struct {
  Game *game;
  int solarChargerComputerIndex;
} SolarChargingComputerThreadArgument;
