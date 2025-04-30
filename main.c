#include "lib/models.h"
#include "lib/vector_ops.h"
#include "raylib.h"
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
    Stores keymaps for each player
    Sequence: Up, Left, Down, Right
*/
int controls[3][7] = {
    {KEY_W, KEY_A, KEY_S, KEY_D, KEY_SPACE, KEY_ONE, KEY_TWO},
    {KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_ENTER, KEY_MINUS, KEY_EQUAL},
};

void drawMessage(Game *game) {
  int messageFontSize = 45;

  DrawRectangle(GetScreenWidth() * 0.5 -
                    (double)MeasureText(game->message, messageFontSize) * 0.5,
                GetScreenHeight() * 0.5 - messageFontSize * 0.5,
                MeasureText(game->message, messageFontSize), messageFontSize,
                (Color){0, 0, 0, 255 * game->messageOpacity});

  DrawText(game->message,
           GetScreenWidth() * 0.5 -
               (double)MeasureText(game->message, messageFontSize) * 0.5,
           GetScreenHeight() * 0.5 - messageFontSize * 0.5, messageFontSize,
           (Color){255, 255, 255, 255 * game->messageOpacity});
}

void updateMessage(Game *game) {
  game->messageOpacity =
      1 - ((float)(game->frameCount - game->messageAddedFrame) /
           (game->messageDuration * game->targetFPS));

  if (game->messageOpacity > 1)
    game->messageOpacity = 1;
  if (game->messageOpacity < 0)
    game->messageOpacity = 0;
}

void showMessage(Game *game, char message[], int fontSize) {
  sprintf(game->message, "%s", message);
  game->messageFontSize = fontSize;
  game->messageAddedFrame = game->frameCount;
  game->messageOpacity = 1;
}

void *computeSolarChargers(void *arg) {

  Game *game = ((SolarChargingComputerThreadArgument *)(arg))->game;
  int j =
      ((SolarChargingComputerThreadArgument *)(arg))->solarChargerComputerIndex;
  free(arg);

  int localFrameCount = -1;

  while (true) {
    if (localFrameCount == game->frameCount) {
      continue;
    }

    localFrameCount = game->frameCount;

    for (int i = j * (game->maxSolarChargers / game->numSolarChargesComputers);
         i <
         (j + 1) * (game->maxSolarChargers / game->numSolarChargesComputers);
         i++) {
      if (game->solarChargers[i].active) {

        // Get charge value
        float chargeValue = ((float)(game->solarChargers[i].height *
                                     game->solarChargers[i].width) /
                             100000) /
                            game->targetFPS;

        // Charge the battery
        pthread_mutex_lock(&game->batteryMutex);
        game->battery += chargeValue;
        pthread_mutex_unlock(&game->batteryMutex);
      }
    }

    if (game->isQuitting) {
      pthread_exit(NULL);
    }
  }
}

void initializeSolarChargers(Game *game) {
  game->solarChargers = calloc(game->maxSolarChargers, sizeof(SolarCharger));

  for (int i = 0; i < game->maxSolarChargers; i++) {
    game->solarChargers[i].active = false;
    game->solarChargers[i].height = 0;
    game->solarChargers[i].width = 0;
    game->solarChargers[i].position = (Vector2){0, 0};
  }

  game->solarChargesComputingThreads =
      (pthread_t *)calloc(game->numSolarChargesComputers, sizeof(pthread_t));

  // Start computing threads for solar charges
  for (int i = 0; i < game->numSolarChargesComputers; i++) {

    SolarChargingComputerThreadArgument *arg =
        malloc(sizeof(SolarChargingComputerThreadArgument));
    arg->solarChargerComputerIndex = i;
    arg->game = game;

    pthread_create(&game->solarChargesComputingThreads[i], NULL,
                   computeSolarChargers, (void *)arg);
  }
}

void buildSolarCharger(Game *game, Vector2 position, int size) {

  if (game->solarCellsCollected < size * 10) {
    return;
  }

  pthread_mutex_lock(&game->solarCellsMutex);
  game->solarCellsCollected -= size * 10;
  pthread_mutex_unlock(&game->solarCellsMutex);

  for (int i = 0; i < game->maxSolarChargers; i++) {
    if (!game->solarChargers[i].active) {
      game->solarChargers[i].active = true;
      game->solarChargers[i].height = 100;
      game->solarChargers[i].width = size * 100;
      game->solarChargers[i].position = position;

      break;
    }
  }
}

// void initializePlayers(Game *game)
// {
//   Color playerColors[] = {YELLOW, BLUE, GREEN, PINK};

//   game->players = (Player *)calloc(sizeof(Player), game->playerCount);

//   for (int i = 0; i < game->playerCount; i++)
//   {
//     game->players[i].size = 50;
//     game->players[i].speed = (float)600 / game->targetFPS;
//     game->players[i].health = 100;
//     game->players[i].color = playerColors[i % game->playerCount];
//     game->players[i].position =
//         (Vector2){0 + i * (20 + game->players[i].size), 0};

//     // Initializing mutexes
//     for (int j = 0; j < game->playerCount; j++)
//     {
//       if (pthread_mutex_init(&game->players[j].mutex, NULL))
//       {
//         exit(0);
//       };
//     }
//   }
// }

// In initializePlayers function:
void initializePlayers(Game *game) {
  Color playerColors[] = {YELLOW, BLUE, GREEN, PINK};

  game->players = (Player *)calloc(sizeof(Player), game->playerCount);

  // Load player textures - make sure the path is correct
  game->playerTextures[0] = LoadTexture("assets/player1.jpeg");
  game->playerTextures[1] = LoadTexture("assets/player2.jpeg");

  // Verify textures loaded correctly
  if (game->playerTextures[0].id == 0 || game->playerTextures[1].id == 0) {
    printf("ERROR: Failed to load player textures!\n");
    exit(1);
  }

  for (int i = 0; i < game->playerCount; i++) {
    game->players[i].size = 50;
    game->players[i].speed = (float)600 / game->targetFPS;
    game->players[i].health = 100;
    game->players[i].color = playerColors[i % game->playerCount];
    game->players[i].position =
        (Vector2){0 + i * (20 + game->players[i].size), 0};

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
    game->enemies[i].active = false;
    game->enemies[i].damage = 5;
    game->enemies[i].speed = (100 + (rand() % 101)) / game->targetFPS;
    game->enemies[i].position = (Vector2){((rand() % range) - (float)range / 2),
                                          (rand() % range) - (float)range / 2};
  }
}

void initializeWaves(Game *game) {
  game->waves = calloc(sizeof(EnemyWave), game->numWaves);

  // Setting up waves
  game->waves[0].numEnemies = 2;
  game->waves[0].waitTime = 10;

  game->waves[1].numEnemies = 2;
  game->waves[1].waitTime = 2;

  game->waves[2].numEnemies = 2;
  game->waves[2].waitTime = 2;

  game->lastWaveFrame = 0;
  game->currentWave = 0;
  game->frameCount = 0;
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

void initializeSolarCells(Game *game) {
  game->solarCells = calloc(game->maxSolarCells, sizeof(SolarCell));

  for (int i = 0; i < game->maxSolarCells; i++) {
    game->solarCells[i].active = false;
    game->solarCells[i].position = (Vector2){0, 0};
    game->solarCells[i].size = 0;
  }
}

void addSolarCell(Game *game, Vector2 position) {

  pthread_mutex_lock(&game->solarCellsMutex);
  for (int i = 0; i < game->maxSolarCells; i++) {

    if (!game->solarCells[i].active) {
      game->solarCells[i].active = true;
      game->solarCells[i].position = position;
      game->solarCells[i].size = 20;
      break;
    }
  }
  pthread_mutex_unlock(&game->solarCellsMutex);
}

void generateSolarCells(Game *game) {
  // Generate n solar cells in random places
  int n = 20;
  int radius = game->mapSize / 2;
  for (int i = 0; i < n; i++) {
    addSolarCell(game, (Vector2){(rand() % radius * 2) - radius,
                                 (rand() % radius * 2) - radius});
  }
}

void drawSolarCells(Game *game) {
  for (int i = 0; i < game->maxSolarCells; i++) {
    if (game->solarCells[i].active) {
      DrawRectangleRounded((Rectangle){game->solarCells[i].position.x -
                                           (float)game->solarCells[i].size / 2,
                                       game->solarCells[i].position.y -
                                           (float)game->solarCells[i].size / 2,
                                       game->solarCells[i].size,
                                       game->solarCells[i].size},
                           1, 1, GRAY);
    }
  }
}

void removeSolarCell(Game *game, int cellIndex) {

  game->solarCells[cellIndex].active = false;
}

void collectSolarCells(Game *game, Player *player) {
  // Check collision against all cells with player

  pthread_mutex_lock(&game->solarCellsMutex);
  for (int i = 0; i < game->maxSolarCells; i++) {
    if (game->solarCells[i].active &&
        CheckCollisionRecs(
            (Rectangle){player->position.x - (float)player->size / 2,
                        player->position.y - (float)player->size / 2,
                        player->size, player->size},
            (Rectangle){game->solarCells[i].position.x -
                            (float)game->solarCells[i].size / 2,
                        game->solarCells[i].position.y -
                            (float)game->solarCells[i].size / 2,
                        game->solarCells[i].size, game->solarCells[i].size})) {

      // Remove Solar Cell
      game->solarCellsCollected++;
      removeSolarCell(game, i);
      printf("REMOVING SOLAR CELL\n");
    }
  }
  pthread_mutex_unlock(&game->solarCellsMutex);
};

void drawEnemies(Game *game) {
  // Drawing all enemies
  for (int i = 0; i < game->maxEnemies; i++) {
    if (game->enemies[i].active) {
      DrawRectangleRounded(
          (Rectangle){
              game->enemies[i].position.x - (float)game->enemies[i].size / 2,
              game->enemies[i].position.y - (float)game->enemies[i].size / 2,
              game->enemies[i].size, game->enemies[i].size},
          1, 1, game->enemies[i].color);
    }
  }
}

void drawSolarChargers(Game *game) {
  for (int i = 0; i < game->maxSolarChargers; i++) {
    if (game->solarChargers[i].active) {
      DrawRectangleRounded(
          (Rectangle){game->solarChargers[i].position.x -
                          (float)game->solarChargers[i].width / 4,
                      game->solarChargers[i].position.y -
                          (float)game->solarChargers[i].height / 4,
                      game->solarChargers[i].width,
                      game->solarChargers[i].height},
          0, 1, WHITE);
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
      game->enemies[i].position =
          (Vector2){((rand() % game->mapSize) - (float)game->mapSize / 2),
                    (rand() % game->mapSize) - (float)game->mapSize / 2};
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
  ViewportThreadArgument *args = (ViewportThreadArgument *)arg;
  Game *game = args->game;
  int viewportIndex = args->viewportIndex;
  Viewport *viewport = &game->viewports[viewportIndex];
  int localFrameCount = -1;
  free(arg);

  while (!game->isQuitting) {
    // Wait for this thread's turn
    sem_wait(&viewport->inputSemaphore);

    // Get current frame count (with mutex protection)
    int currentFrame = game->frameCount;

    // Skip processing if we already handled this frame
    if (localFrameCount == currentFrame) {
      sem_post(&game->viewports[(viewportIndex + 1) % game->playerCount]
                    .inputSemaphore);
      continue;
    }

    localFrameCount = currentFrame;

    // Process input
    Vector2 direction = {0, 0};
    const int controlScheme = viewportIndex % 2; // Use 2 control schemes

    // Movement controls (using simplified array access)
    if (IsKeyDown(controls[controlScheme][0]))
      direction.y = -1; // Up
    if (IsKeyDown(controls[controlScheme][1]))
      direction.x = -1; // Left
    if (IsKeyDown(controls[controlScheme][2]))
      direction.y = 1; // Down
    if (IsKeyDown(controls[controlScheme][3]))
      direction.x = 1; // Right

    // Build Solar Charger Small
    if (IsKeyPressed(controls[controlScheme][5])) {
      pthread_mutex_lock(&viewport->player->mutex);
      buildSolarCharger(game, viewport->player->position, 1);
      pthread_mutex_unlock(&viewport->player->mutex);
    }

    // Build Solar Charger Large
    if (IsKeyPressed(controls[controlScheme][6])) {
      pthread_mutex_lock(&viewport->player->mutex);
      buildSolarCharger(game, viewport->player->position, 2);
      pthread_mutex_unlock(&viewport->player->mutex);
    }

    // Shoot action
    if (IsKeyPressed(controls[controlScheme][4])) {

      float enemyHealth = 0.1;

      if (game->battery > enemyHealth) {

        pthread_mutex_lock(&viewport->player->mutex);
        int closestEnemy =
            getClosestEnemyIndex(game, viewport->player->position);
        pthread_mutex_unlock(&viewport->player->mutex);

        if (closestEnemy != -1) {
          pthread_mutex_lock(&game->batteryMutex);

          killEnemy(game, closestEnemy);
          game->battery -= enemyHealth;

          pthread_mutex_unlock(&game->batteryMutex);
        }
      }
    }

    // Camera controls (global)
    if (viewportIndex == 0) { // Only process these once
      if (IsKeyPressed(KEY_EQUAL)) {
        for (int i = 0; i < game->playerCount; i++) {
          game->viewports[i].camera->zoom += 0.25;
        }
      }
      if (IsKeyPressed(KEY_MINUS)) {
        for (int i = 0; i < game->playerCount; i++) {
          game->viewports[i].camera->zoom -= 0.25;
        }
      }
    }

    // Check if solar cell collected
    collectSolarCells(game, viewport->player);

    // Apply movement if game isn't paused
    if (!game->paused && (direction.x != 0 || direction.y != 0)) {
      direction = normalizeVector2(direction);

      Vector2 velocity = {direction.x * viewport->player->speed,
                          direction.y * viewport->player->speed};

      int boundx = game->mapSize / 2, boundy = game->mapSize / 2;

      if (velocity.x < 0 && viewport->player->position.x < -boundx) {
        velocity.x = 0;
      }

      if (velocity.x > 0 && viewport->player->position.x > boundx) {
        velocity.x = 0;
      }

      if (velocity.y < 0 && viewport->player->position.y < -boundy) {
        velocity.y = 0;
      }

      if (velocity.y > 0 && viewport->player->position.y > boundy) {
        velocity.y = 0;
      }

      pthread_mutex_lock(&viewport->player->mutex);
      viewport->player->position.x += velocity.x;
      viewport->player->position.y += velocity.y;

      pthread_mutex_unlock(&viewport->player->mutex);
    }

    // Pass control to next thread
    sem_post(&game->viewports[(viewportIndex + 1) % game->playerCount]
                  .inputSemaphore);
  }

  return NULL;
}

// void drawPlayers(Game *game)
// {
//   for (int i = 0; i < game->playerCount; i++)
//   {
//     pthread_mutex_lock(&game->players[i].mutex);
//     DrawRectangleRounded(
//         (Rectangle){
//             game->players[i].position.x - (float)game->players[i].size / 2,
//             game->players[i].position.y - (float)game->players[i].size / 2,
//             game->players[i].size, game->players[i].size},
//         0.5, 1, game->players[i].color);
//     pthread_mutex_unlock(&game->players[i].mutex);
//   }
// }

// In drawPlayers function:
void drawPlayers(Game *game) {
  for (int i = 0; i < game->playerCount; i++) {
    pthread_mutex_lock(&game->players[i].mutex);

    // Determine if the player is moving left or right
    float direction = game->players[i].position.x < 0 ? -1.0f : 1.0f;

    // Flip the texture horizontally if moving left
    Rectangle sourceRect = {
        0, 0,
        direction * game->playerTextures[i % 2]
                        .width, // Flip width if direction is negative
        game->playerTextures[i % 2].height};

    DrawTexturePro(
        game->playerTextures[i % 2], sourceRect,
        (Rectangle){
            game->players[i].position.x - (float)game->players[i].size / 2,
            game->players[i].position.y - (float)game->players[i].size / 2,
            game->players[i].size, game->players[i].size},
        (Vector2){0, 0}, 0.0f, WHITE);

    pthread_mutex_unlock(&game->players[i].mutex);
  }
}

void drawMap(Game *game) {

  DrawRectangleLines(-game->mapSize / 2, -game->mapSize / 2, game->mapSize,
                     game->mapSize, GREEN);
}

void win(Game *game) {
  game->paused = true;
  game->gameWon = true;
}

void handleGameOver(Game *game) {
  for (int i = 0; i < game->playerCount; i++) {
    if (game->players[i].health < 0) {
      game->paused = true;
      game->gameOver = true;
    }
  }
}

void generateEnemies(Game *game) {
  if (game->currentWave < game->numWaves) {
    if (game->frameCount >
        game->lastWaveFrame +
            game->waves[game->currentWave].waitTime * game->targetFPS) {

      // Unleash the enemies for this wave
      char message[256];
      sprintf(message, "WAVE %d begins!", game->currentWave);
      showMessage(game, message, 45);

      addEnemies(game, game->waves[game->currentWave].numEnemies);
      game->lastWaveFrame = game->frameCount;
      game->currentWave++;
    }
  } else {
    if (game->enemyCount <= 0)
      win(game);
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

    // Draw Borders
    drawMap(game);

    // Draw Solar Chargers
    drawSolarChargers(game);

    // Draw solar cells
    drawSolarCells(game);

    // Draw all enemies
    drawEnemies(game);

    // Drawing all players
    drawPlayers(game);

    // Draw player health
    char playerHealthText[128];
    int playerHealthFontSize = 18;
    pthread_mutex_lock(&game->players[i].mutex);
    sprintf(playerHealthText, "Player %d Health: %.1f", i + 1,
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
  int enemiesAliveFontSize = 25;

  DrawText(enemiesAliveText, GetScreenWidth() * 0.01, GetScreenHeight() * 0.02,
           enemiesAliveFontSize, WHITE);

  // Draw battery left
  char batteryText[128];
  sprintf(batteryText, "Battery: %.1f Volts", game->battery);
  int batteryFontSize = 25;

  DrawText(batteryText, GetScreenWidth() * 0.01,
           GetScreenHeight() * 0.03 + enemiesAliveFontSize, batteryFontSize,
           WHITE);

  // Draw Solar Cells Collected
  char solarCellsText[128];
  sprintf(solarCellsText, "Solar Cells: %d", game->solarCellsCollected);
  int solarCellsFontSize = 25;

  DrawText(solarCellsText, GetScreenWidth() * 0.01,
           GetScreenHeight() * 0.06 + solarCellsFontSize, solarCellsFontSize,
           WHITE);

  // Draw wave number
  char waveNumberText[128];
  sprintf(waveNumberText, "Current Wave: %d", game->currentWave);
  int waveNumberFontSize = 25;

  DrawText(waveNumberText, GetScreenWidth() * 0.01,
           GetScreenHeight() * 0.09 + waveNumberFontSize, waveNumberFontSize,
           WHITE);

  // Draw Time to next wave
  int secondsToNextWave =
      ((game->waves[game->currentWave].waitTime * game->targetFPS +
        game->lastWaveFrame) -
       game->frameCount) /
      game->targetFPS;

  char timeToNextWaveText[128];
  if (secondsToNextWave >= 0) {
    sprintf(timeToNextWaveText, "Time to next wave: %ds", secondsToNextWave);
  } else {
    sprintf(timeToNextWaveText, "Time to next wave: Inifinity");
  }
  int timeToNextWaveFontSize = 25;

  DrawText(timeToNextWaveText, GetScreenWidth() * 0.01,
           GetScreenHeight() * 0.12 + timeToNextWaveFontSize,
           timeToNextWaveFontSize, WHITE);

  // Draw message shown to player
  drawMessage(game);
}

// void killViewports(Game *game)
// {
//   game->isQuitting = true;
//   for (int i = 0; i < game->playerCount; i++)
//   {
//     // Join all threads
//     pthread_join(game->viewports[i].thread, NULL);

//     // Unloading all render textures out of the GPU
//     UnloadRenderTexture(*game->viewports[i].renderTexture);

//     // Destroy semaphores
//     sem_destroy(&game->viewports[i].inputSemaphore);
//   }
// }
// Add texture unloading in killViewports function
// In killViewports function:
void killViewports(Game *game) {
  game->isQuitting = true;
  for (int i = 0; i < game->playerCount; i++) {
    pthread_join(game->viewports[i].thread, NULL);
    UnloadRenderTexture(*game->viewports[i].renderTexture);
    sem_destroy(&game->viewports[i].inputSemaphore);
  }

  // Unload player textures
  UnloadTexture(game->playerTextures[0]);
  UnloadTexture(game->playerTextures[1]);
}
int main(int argc, char **args) {

  srand(time(NULL));
  InitWindow(0, 0, "Thread Wars");

  if (!IsWindowFullscreen()) {
    ToggleFullscreen();
  }

  Game game;

  game.playerCount = 2;

  game.messageDuration = 1;

  game.maxEnemies = 300;
  game.enemyCount = 0;

  game.solarCells = 0;
  game.maxSolarCells = 100;
  game.solarCellsCollected = 0;

  game.maxSolarChargers = 300;
  game.numSolarChargesComputers = 5;

  game.battery = 0;

  game.mapSize = 1000;

  game.targetFPS = 60;
  game.paused = false;
  game.isQuitting = false;
  game.gameOver = false;
  game.gameWon = false;

  game.numWaves = 3;
  game.currentWave = 0;

  initializeWaves(&game);
  initializePlayers(&game);
  initializeEnemies(&game);
  initializeSolarChargers(&game);
  initializeViewports(&game);
  initializeSolarCells(&game);

  pthread_mutex_init(&game.enemyCountMutex, NULL);
  pthread_mutex_init(&game.batteryMutex, NULL);
  pthread_mutex_init(&game.solarCellsMutex, NULL);

  SetTargetFPS(game.targetFPS);

  // Creating viewport threads
  for (int i = 0; i < game.playerCount; i++) {
    ViewportThreadArgument *args = malloc(sizeof(ViewportThreadArgument));
    args->viewportIndex = i;
    args->game = &game;

    pthread_create(&game.viewports[i].thread, NULL, updatePlayer, (void *)args);
  }

  while (!WindowShouldClose()) {

    if (game.frameCount % (game.targetFPS * 5) == 0) {
      printf("GENERATED SOLAR CELLS\n");
      generateSolarCells(&game);
    }

    // Pausing
    if (IsKeyPressed(KEY_P)) {
      if (!game.gameOver) {
        game.paused = !game.paused;
      }
    }

    // Adding enemies
    if (IsKeyPressed(KEY_BACKSPACE)) {
      addEnemies(&game, 5);
    }

    handleGameOver(&game);

    // Update
    if (!game.paused) {
      generateEnemies(&game);

      updateMessage(&game);
      updateEnemies(&game);

      game.frameCount++;
    }

    BeginDrawing();
    // Drawing everything to screen
    draw(&game);

    if (game.paused && !game.gameOver && !game.gameWon) {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenWidth(), Fade(BLACK, 0.7));
      char enemiesAliveText[] = "PAUSED";
      int fontSize = GetScreenHeight() * 0.05;
      DrawText(enemiesAliveText,
               GetScreenWidth() / 2 -
                   MeasureText(enemiesAliveText, fontSize) / 2,
               GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
    }

    if (game.gameOver) {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenWidth(), Fade(BLACK, 0.7));
      char gameOverText[] = "GAME OVER :(";
      int fontSize = GetScreenHeight() * 0.05;
      DrawText(gameOverText,
               GetScreenWidth() / 2 - MeasureText(gameOverText, fontSize) / 2,
               GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
    }

    if (game.gameWon) {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenWidth(), Fade(BLACK, 0.7));
      char wonText[] = "YOU SURVIVED :)";
      int fontSize = GetScreenHeight() * 0.05;
      DrawText(wonText,
               GetScreenWidth() / 2 - MeasureText(wonText, fontSize) / 2,
               GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
    }

    EndDrawing();
  }

  game.isQuitting = true;

  // // killSolarChargerComputers
  // for (int i = 0; i < game.numSolarChargesComputers; i++) {
  //   pthread_join(*game.solarChargesComputingThreads, NULL);
  // }

  // killViewports(&game);

  CloseWindow();

  return 0;
}