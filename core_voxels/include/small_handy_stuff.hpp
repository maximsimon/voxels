// Small handy stuff. Convient functions that shorten the main code by few lines

#ifndef SMALL_HANDY_STUFF_H
#define SMALL_HANDY_STUFF_H

#include "raylib.h"
#include "raymath.h"

Vector3 getUpDirection();
Vector3 getDownDirection();
Vector3 getRightDirection(Camera camera);
Vector3 getLeftDirection(Camera camera);
Vector3 getBackDirection(Camera camera);
Vector3 getForwardDirection(Camera camera);

#endif
