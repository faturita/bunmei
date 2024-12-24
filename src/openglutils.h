/*
 * openglutils.h
 *
 *  Created on: Jan 16, 2011
 *      Author: faturita
 */

#ifndef OPENGLUTILS_H_
#define OPENGLUTILS_H_


#include <cassert>
#ifdef __linux
#include <GL/glut.h>
#elif __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#endif

#include <vector>
#include "math/vec3f.h"
#include "imageloader.h"

void placeMark(float x, float y, int sizex, int sizey, GLuint _texture);
void placeMark(float x, float y, int sizex, int sizey, const char* modelName);
void placeMark(float x, float y, int size, const char* modelName);
GLuint preloadTexture(const char* modelName);



void CheckGLError();
GLuint loadTexture(Image* image) ;
void drawLine(float x, float y, float z, float red, float green, float blue);
void drawLine(float x, float y, float z,float red, float green, float blue, float linewidth);
void drawArrow();
void drawArrow(float scale);
void drawArrow(float x, float y, float z);
void drawArrow(float x, float y, float z,float red, float green, float blue);
void drawArrow(float x, float y, float z,float red, float green, float blue, float linewidth);
void doTransform (float pos[3], float R[12]);
void doTransform(float R[12]);
void drawRectangularBox(float width, float height, float length);
void drawRectangularBox(float width, float height, float length, GLuint _textureId);
void drawRectangularBox(float width, float height, float length, Vec3f green, Vec3f yellow, Vec3f blue, Vec3f magenta);
void drawTheRectangularBox(GLuint _textureId, float xx, float yy, float zz);
void drawTexturedBox(GLuint _textureId, float xx, float yy, float zz);
void drawBox(GLuint texturedId, float xx, float yy, float zz);
void drawBox(float xx, float yy, float zz);

void drawRedBox(float x, float y, int sizex, int sizey);
void drawBlueBox(float x, float y, int sizex, int sizey);

void drawBoxIsland(GLuint _textureId, float xx, float yy, float zz, float side,float height);
void drawBoxIsland(float xx, float yy, float zz, float side, float height);

void drawFloor(float x, float y, float z);

void drawLightning();

void drawSky (float x,float y, float z);

void initTextures();

float getFPS();

void getScreenLocation(float &winX, float &winY, float &winZ, float xx, float zz, float yy);

void Draw_Texture(double x, double y, double z, double width, double height, double Angle, GLuint texture);

GLuint preloadCityTexture(const char* modelName, int red, int green, int blue);
GLuint preloadUnitTexture(const char* modelName, int red, int green, int blue);

#endif /* OPENGLUTILS_H_ */
