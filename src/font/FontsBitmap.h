#ifndef FONTSBITMAP_H
#define FONTSBITMAP_H

#include "../openglutils.h"

GLuint preloadFont(const char* fontName);
void preloadFonts();
void placeLetter(float x, float y, const char* letter);
void placeWord(float x, float y, const char* word);

#endif   // FONTSBITMAP_H
