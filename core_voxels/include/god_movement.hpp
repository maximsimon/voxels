// CAMERA (edit mode) MOVEMENT (translation, rotation)

#ifndef CAMERA_MOVEMENT_H
#define CAMERA_MOVEMENT_H

#include "raylib.h"
#include "raymath.h"
#include "map.hpp"
#include "data_types.hpp"

void CheckMovementEdit(Camera *camera);		// Check for keyboard keys that move
void CameraMoveEdit(Camera *camera, Vector3 direction);		// Translate camera
void CameraMoveUpDownEdit(Camera *camera, Vector3 direction);		// Translate camera
void CameraRotateEdit(Camera *camera, int HEADING);			// Rotate camera

#endif

