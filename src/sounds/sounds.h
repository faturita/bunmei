#ifndef SOUNDS_H
#define SOUNDS_H


#ifdef __linux
#define PLAYSOUNDCOMMAND "mpg321"
#elif __APPLE__
#define PLAYSOUNDCOMMAND "afplay"
#include <GLUT/glut.h>
#endif

#include "../math/yamathutil.h"

#define SOUND_DISTANCE_LIMIT 1000.0


void initSound();
void clearSound();

void explosion(Vec3f source);
void blocked();

void intro();


#endif // SOUNDS_H