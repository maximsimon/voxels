// CAMERA (edit mode) MOVEMENT (translation, rotation)

#ifndef CAMERA_MOVEMENT_H
#define CAMERA_MOVEMENT_H

#include "raylib.h"
#include "raymath.h"
#include "map.h"
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void CheckMovementEdit(Camera *camera, chunkMap map);		// Check for keyboard keys that move
void CameraMoveEdit(Camera *camera, Vector3 direction);		// Translate camera
void CameraMoveUpDownEdit(Camera *camera, Vector3 direction);		// Translate camera
void CameraRotateEdit(Camera *camera, int HEADING);			// Rotate camera

#ifdef __cplusplus
}
#endif

#endif

