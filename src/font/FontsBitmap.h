#ifndef FONTSBITMAP_H
#define FONTSBITMAP_H

#include "../openglutils.h"

GLuint preloadFont(const char* fontName);
void preloadFonts();
void placeWord(float x, float y, int sizex, int sizey, const char* word);

#endif   // FONTSBITMAP_H
