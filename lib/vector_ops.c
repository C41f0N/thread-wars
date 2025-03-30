
#include <math.h>
#include <raylib.h>

float getDistanceBetweenVectors(Vector2 v1, Vector2 v2) {
  return sqrtf(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2));
}

float getVectorMagnitude(Vector2 v) { return sqrtf(pow(v.x, 2) + pow(v.y, 2)); }

Vector2 normalizeVector2(Vector2 v) {
  float magnitude = getVectorMagnitude(v);
  if (magnitude == 0) {
    return (Vector2){0, 0};
  }
  return (Vector2){v.x / magnitude, v.y / magnitude};
}

Vector2 getDirectionVector2s(Vector2 v1, Vector2 v2) {
  float distance = getDistanceBetweenVectors(v1, v2);
  if (distance == 0) {
    return (Vector2){0, 0};
  }

  return (Vector2){(v2.x - v1.x) / getDistanceBetweenVectors(v1, v2),
                   (v2.y - v1.y) / getDistanceBetweenVectors(v1, v2)};
}