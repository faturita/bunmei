#include <vector>

#include <iostream>
#include <unordered_map>
#include "openglutils.h"
#include "lodepng.h"
#include "map.h"


// 1200 x 800
// 1920 x 1080
int width = 1200;
int height = 800;
int mapzoom=1;

float cx,cy;

std::unordered_map<std::string, GLuint> maptextures;

void zoommapin()
{
    if (mapzoom < 11)
        mapzoom++;
}

void zoommapout()
{
    if (mapzoom>1)
        mapzoom--;
}

void centermap(int ccx, int ccy)
{
    // @FIXME: Parametrize all the resolution values.
    int xsize = 1200/mapzoom;
    int ysize = 800/mapzoom;

    // Screen width and height come from OpenGL.
    cx = (int)(ccx*(xsize)/(float)width)+cx-xsize/2; // 1440
    cy = (int)(ccy*(ysize)/(float)height)+cy-ysize/2;  // 900
}

void placeMark2(float x, float y, int sizex, int sizey, const char* modelName)
{
    GLuint _texture;

    if (maptextures.find(std::string(modelName)) == maptextures.end())
    {
        // @FIXME: This means that the image is loaded every time this mark is rendered, which is wrong.
        //Image* image = loadBMP(modelName);
        //_texture = loadTexture(image);
        //delete image;

        unsigned char *img;

        unsigned w,h;

        lodepng_decode_file(&img, &w, &h, modelName, LCT_RGBA, 8);


        Image image((char *)img, w, h);

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     w,h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     img);
        //delete image;


        maptextures[std::string(modelName)]=_texture;

    } else {
        _texture = maptextures[std::string(modelName)];
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glColor4f(1.0f, 1.0f, 1.0f,1.0f);


    glBegin(GL_QUADS);
    //Front face
    glNormal3f(0.0, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-sizex / 2 + x, -sizey / 2 + y, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(sizex / 2 + x, -sizey / 2 + y, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(sizex / 2 + x, sizey / 2 + y, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-sizex / 2 + x, sizey / 2 + y, 0);

    glEnd();

    // @NOTE: This is very important.
    glDisable(GL_TEXTURE_2D);
}


void placeCity(float x, float y, int size, const char* modelName)
{
    GLuint _texture;

    if (maptextures.find(std::string(modelName)) == maptextures.end())
    {
        // @FIXME: This means that the image is loaded every time this mark is rendered, which is wrong.
        //Image* image = loadBMP(modelName);
        //_texture = loadTexture(image);
        //delete image;

        unsigned char *img;

        unsigned w,h;

        lodepng_decode_file(&img, &w, &h, modelName, LCT_RGBA, 8);

        Image image((char *)img, w, h);

        printf("Sizeof w=%d,h=%d, image: %d\n", w,h, sizeof(img));

        for(int i=0;i<w;i++)
            for (int j=0;j<h;j++)
            {
                if (!(img[(i*h+j)*4+0]==0 && img[(i*h+j)*4+1]==0 && img[(i*h+j)*4+2] == 0))
                {
                    printf("Color of the unit %d,%d,%d\n",img[(i*h+j)*4+0],img[(i*h+j)*4+1],img[(i*h+j)*4+2] );
                    img[(i*h+j)*4+0] = 255;
                    img[(i*h+j)*4+1] = 0;
                    img[(i*h+j)*4+2] = 0;

                    img[(i*h+j)*4+3] = 255;
                }
            }

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     w,h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     img);
        //delete image;


        maptextures[std::string(modelName)]=_texture;

    } else {
        _texture = maptextures[std::string(modelName)];
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);


    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


    glColor4f(1.0f, 1.0f, 1.0f,1.0f);

    //glColor3f(1.0,0,0);
    glBegin(GL_QUADS);
    //Front face
    glNormal3f(0.0, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(size / 2 + x, size / 2 + y, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-size / 2 + x, size / 2 + y, 0);

    glEnd();

    // @NOTE: This is very important.
    glDisable(GL_TEXTURE_2D);
}


void placeUnit(float x, float y, int size, const char* modelName)
{
    GLuint _texture;

    if (maptextures.find(std::string(modelName)) == maptextures.end())
    {
        // @FIXME: This means that the image is loaded every time this mark is rendered, which is wrong.
        //Image* image = loadBMP(modelName);
        //_texture = loadTexture(image);
        //delete image;

        unsigned char *img;

        unsigned w,h;

        lodepng_decode_file(&img, &w, &h, modelName, LCT_RGBA, 8);

        Image image((char *)img, w, h);

        printf("Sizeof w=%d,h=%d, image: %d\n", w,h, sizeof(img));

        for(int i=0;i<w;i++)
            for (int j=0;j<h;j++)
            {
                if (img[(i*h+j)*4+0]==96 && img[(i*h+j)*4+1]==224 && img[(i*h+j)*4+2] == 100)
                {
                    //printf("Color of the unit %d,%d,%d\n",img[(i*h+j)*4+0],img[(i*h+j)*4+1],img[(i*h+j)*4+2] );
                    img[(i*h+j)*4+0] = 255;
                    img[(i*h+j)*4+1] = 0;
                    img[(i*h+j)*4+2] = 0;

                    //img[(i*h+j)*4+3] = 255;
                }
            }

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     w,h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     img);
        //delete image;


        maptextures[std::string(modelName)]=_texture;

    } else {
        _texture = maptextures[std::string(modelName)];
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);


    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


    glColor4f(1.0f, 1.0f, 1.0f,1.0f);

    //glColor3f(1.0,0,0);
    glBegin(GL_QUADS);
    //Front face
    glNormal3f(0.0, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(size / 2 + x, size / 2 + y, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-size / 2 + x, size / 2 + y, 0);

    glEnd();

    // @NOTE: This is very important.
    glDisable(GL_TEXTURE_2D);
}

void placeThisUnit(float y, float x, int size, const char* modelName)
{
    placeUnit(600+16*y,0+16*x,size,modelName);
}

void placeCity(float y, float x)
{
    placeCity(600+16*y,0+16*x,16,"assets/assets/map/city.png");
}


void placeMark(float x, float y, int size, const char* modelName)
{
    GLuint _texture;

    if (maptextures.find(std::string(modelName)) == maptextures.end())
    {
        // @FIXME: This means that the image is loaded every time this mark is rendered, which is wrong.
        //Image* image = loadBMP(modelName);
        //_texture = loadTexture(image);
        //delete image;

        unsigned char *img;

        unsigned w,h;

        lodepng_decode_file(&img, &w, &h, modelName, LCT_RGBA, 8);

        Image image((char *)img, w, h);

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     w,h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     img);
        //delete image;


        maptextures[std::string(modelName)]=_texture;

    } else {
        _texture = maptextures[std::string(modelName)];
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);


    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);




    glColor4f(1.0f, 1.0f, 1.0f,1.0f);

    //glColor3f(1.0,0,0);
    glBegin(GL_QUADS);
    //Front face
    glNormal3f(0.0, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(size / 2 + x, size / 2 + y, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-size / 2 + x, size / 2 + y, 0);

    glEnd();

    // @NOTE: This is very important.
    glDisable(GL_TEXTURE_2D);
}

void place(float y, float x, int size, const char* modelName)
{
    placeMark(600+16*y,0+16*x,size,modelName);
}


void drawMap()
{
    // This will make things dark.

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int xsize = width/mapzoom;
    int ysize = height/mapzoom;

    if (mapzoom==1)
    {
        cx = width/2;
        cy = height/2;
    }


    glOrtho(cx-xsize/2, cx+xsize/2, cy+ysize/2, cy-ysize/2, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1.0f, 1.0f, 1.0f, 1);
    glDisable(GL_DEPTH_TEST);
    glRotatef(180.0f,0,0,1);
    glRotatef(180.0f,0,1,0);



    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //char str[256];
    //sprintf (str, "Civilization");
    // width, height, 0 0 upper left
    //drawString(0,-30,1,str,0.2f,1.0f,1.0f,1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glTranslatef(0, -400, 1);

        glDepthMask(GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for(int x=-30;x<30;x++)
        {
            for(int y=-40;y<40;y++)
            {
                place(y,x,16,"assets/assets/terrain/ocean.png");
            }
        }

        for(int x=-10;x<=10;x++)
        {
            for(int y=-2;y<=2;y++)
            {
                place(y,x,16,"assets/assets/terrain/land.png");
            }
        }

        int y;
        int x=10;
        for (int y=-2;y<=2;y++)
        {
            placeMark(600+16*y-4, 0+16*x+8,    8,"assets/assets/terrain/coast_n2.png");
            placeMark(600+16*y+4, 0+16*x+8,    8,"assets/assets/terrain/coast_n2.png");
        }

        x=-10;
        for (int y=-2;y<=2;y++)
        {
            placeMark(600+16*y-4, 0+16*x-8,    8,"assets/assets/terrain/coast_s2.png");
            placeMark(600+16*y+4, 0+16*x-8,    8,"assets/assets/terrain/coast_s2.png");
        }

        y=-2;
        for (int x=-10;x<=10;x++)
        {
            placeMark(600+16*y-8, 0+16*x-4,    8,"assets/assets/terrain/coast_w2.png");
            placeMark(600+16*y-8, 0+16*x+4,    8,"assets/assets/terrain/coast_w2.png");
        }

        y=2;
        for (int x=-10;x<=10;x++)
        {
            placeMark(600+16*y+8, 0+16*x-4,    8,"assets/assets/terrain/coast_e2.png");
            placeMark(600+16*y+8, 0+16*x+4,    8,"assets/assets/terrain/coast_e2.png");
        }

        place(0,0,16,"assets/assets/terrain/river_e.png");
        place(1,0,16,"assets/assets/terrain/river_ew.png");
        place(2,0,16,"assets/assets/terrain/river_ew.png");
        place(3,0,16,"assets/assets/terrain/river_mouth_w.png");


        placeCity(-2,-2);

        int X=0,Y=0;
        static int count=0;
        if (count++ % 100 < 50)
            placeThisUnit(Y,X,16,"assets/assets/units/settlers.png");


        glDisable(GL_BLEND);

    } glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

}




