#include "openglutils.h"
#include "font/DrawFonts.h"
#include "lodepng.h"
#include "usercontrols.h"
#include "math/yamathutil.h"
#include "units/Unit.h"
#include "Faction.h"
#include "gamekernel.h"
#include "messages.h"
#include "hud.h"

extern std::unordered_map<std::string, GLuint> maptextures;


extern int year;

extern Controller controller;
extern std::unordered_map<int, Unit*> units;
extern std::vector<Faction*> factions;

extern std::vector<Message> messages;


void placeMark4(float x, float y, int size, const char* modelName)
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

void drawHUD()
{
    // This will make things dark.

    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1200, 800, 0, -1, 1);            // @NOTE: This is the size of the HUD screen (FIXED), it goes from x=[0,1200] , y=[-400,400]
    glMatrixMode(GL_MODELVIEW);                 //  the HUD screen is independent of the map screen.  It is an overly on top of it.
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_CURRENT_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1);
    glDisable(GL_DEPTH_TEST);
    glRotatef(180.0f,0,0,1);
    glRotatef(180.0f,0,1,0);

    int aimc=0,crossc=0;

    char str[256];

    sprintf (str, "Bunmei - %s", factions[controller.faction]->name);
    // width, height, 0 0 upper left
    drawString(0,-30,1,str,0.2f);

    char msg[256];
    if (year<0)
        sprintf (msg, "Year: %4d BC",-year);
    else
        sprintf (msg, "Year: %4d AD",year);

    sprintf (str,msg);
    drawString(0,-60,1,str,0.2f);


    sprintf (str, "Population:%d",factions[controller.faction]->pop);
    drawString(0,-90,1,str,0.2f);

    if (controller.controllingid != CONTROLLING_NONE)
    {
        sprintf (str, "(%s)",units[controller.controllingid]->name);
        drawString(0,-120,1,str,0.2f);

        sprintf (str, "(%d, %d)",units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
        drawString(0,-150,1,str,0.2f);
    }

    placeMark4(10,-180,7*3,"assets/assets/city/bulb.png");


    //sprintf (str, "Military Advisor: enemy units are coming from the south.");
    //drawString(0,-700,1,str,0.1f);


    static int mbrefresher = 1000;
    // Message board
    if (messages.size()>0)
    {
        int msgonboard=0;
        for(size_t i=0;i<messages.size();i++)
        {
            if (messages[i].faction == controller.faction || messages[i].faction == -1)
            {
                std::string line = messages[i].msg;
                if (msgonboard==0)
                    drawString(0,-700-msgonboard*25,1,(char*)line.c_str(),0.1f,1.0f,1.0f,0.0f);
                else
                    drawString(0,-700-msgonboard*25,1,(char*)line.c_str(),0.1f);
                msgonboard++;
            }
        }

        if (mbrefresher--<=0)
        {
            while (messages.size()>5)
            {
                Message ms = messages.back();
                if (ms.year==0)
                {
                    ms.year = year;
                }
                //msgboardfile << ms.faction <<  "|" << ms.timer << ":" << ms.msg <<  std::endl ;
                //msgboardfile.flush();

                messages.pop_back();
                mbrefresher = 1000;
            }
        }

    }


    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}
