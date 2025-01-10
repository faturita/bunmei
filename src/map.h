#ifndef MAP_H
#define MAP_H

#include "openglutils.h"
#include "math/yamathutil.h"
#include "coordinate.h"
#include "mapmodel.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800


void drawMap();
void drawIntro();

void resetzoom();
void zoommapin();
void zoommapout();
// Get Screen X,Y coordinate where the user clicked and convert it to opengl coordinates.
void centermap(int ccx, int ccy);
void centermapinmap(int lat, int lon);
coordinate getCurrentCenter();
coordinate convertToMap(int ccx, int ccy, int gridsize);

void unfog(int lat, int lon);

void drawUnitsAndCities();
void openCityScreen();

void placeFloat(float x, float y, int sizex, int sizey, GLuint _texture);
void place(int x, int y, int sizex, int sizey, const char* modelName);
void place(int x, int y, int sizex, int sizey, GLuint _texture);
void place(int x, int y, int size, const char* modelName);
void placeTile(int x, int y, const char* modelName);
void placeTile(int x, int y, int size, const char* modelName);
void placeThisTile(int lat, int lon, int size, const char* filename);
void placeThisUnit(float flat, float flon, int size, const char* modelName, int red=255, int green=0, int blue=0);
void placeThisCity(int lat, int lon, int red, int green, int blue);

#endif   // MAP_H