#ifndef GAMEKERNEL_H
#define GAMEKERNEL_H

#include "mapio.h"

class Unit;

void initMap();
void initFactions();
void initUnits();          // the starting units for every faction
void initResources();

void initWorldModelling();
void worldStep(int value);
void loadWorldModelling();

#endif // GAMEKERNEL_H