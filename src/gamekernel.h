#ifndef GAMEKERNEL_H
#define GAMEKERNEL_H

void initMap();
void initFactions();
void initResources();

void initWorldModelling();
void worldStep(int value);
void loadWorldModelling();

#endif // GAMEKERNEL_H