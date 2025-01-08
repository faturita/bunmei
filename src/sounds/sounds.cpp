#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <unordered_map>

#include <assert.h>

#include "soundtexture.h"

#include "sounds.h"

#include "../camera.h"
#include "../profiling.h"


//extern  Camera camera;
extern bool mute;

//std::unordered_map<std::string, SoundTexture*> soundtextures;
SoundTexture s;

struct SoundOrder {
    Vec3f source;
    char soundname[256];
} soundelement ;

bool newsound = false;

void playthissound_(Vec3f source, char fl[256]);

void playthissound(Vec3f source, char fl[256])
{
    soundelement.source = source;
    strncpy(soundelement.soundname, fl, 256);

    newsound = true;
}

void * sound_handler(void *arg)
{
    int sd;

    sd = *((int*)arg);

    while (!mute && !s.interrupt)
    {
        if (newsound)
        {
            playthissound_(soundelement.source, soundelement.soundname);
            newsound = false;
        }

        usleep(100);
    }
}

// @NOTE: Stk is very tricky, particularly in linux.  So I have found that I cannot work on two buffers at the same time and I needed to serialize the access.
//   But this also makes the system so much slower, so it is just better to use a separate thread to handle the sound.
//   This way works much better and there is really no penalty in performance.
void initSound()
{
    //SoundTexture* so = new SoundTexture();
    //so->init("sounds/takeoff.wav");
    //soundtextures["takeoff"] = so;

    //so = new SoundTexture();
    //so->init("sounds/gunshot.wav");
    //soundtextures["gunshot"] = so;

    pthread_t th;

    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    int sd=29;

    pthread_create(&th, &attr, &sound_handler, (void*)&sd);
    usleep(3000);  

}

void clearSound()
{
    if (!mute)
    {
        s.interrupt = true;
        s.close();
    }
}

void playthissound(char fl[256])
{
    try {
        if (!mute) {
            while (!s.done)
            {
                s.interrupt = true;
                Stk::sleep( 0 );
            }
            //static SoundTexture s;
            s.init(fl);
            s.amplitude = 1.0;
            s.play();
        }
    }  catch (StkError) {
        dout << "Sound error reported, but ignored." << std::endl;
    }

}

void playthissound_(Vec3f source, char fl[256])
{
    // @NOTE: Use the camera location to determine if the sound should be reproduced or not
    //   and with which intensity.
    try {
        if (!mute) {
            Vec3f dist = source - Vec3f(0,0,0);//camera.pos;
            if (dist.magnitude()<SOUND_DISTANCE_LIMIT)
            {
                StkFloat amplitude = SOUND_DISTANCE_LIMIT-dist.magnitude() / SOUND_DISTANCE_LIMIT;
                amplitude = 1.0;
                while (!s.done)
                {
                    s.interrupt = true;
                    Stk::sleep( 0 );
                }
                //static SoundTexture s;
                s.init(fl);
                s.amplitude = amplitude;
                s.play();
            }
        }
    }  catch (StkError) {
        dout << "Sound error reported, but ignored." << std::endl;
    }

}




void firesound(int times)
{
    for(int i=0;i<times;i++)
        printf ("%c", 7);

extern bool mute;

//std::unordered_map<std::string, SoundTexture*> soundtextures;
SoundTexture s;
}


void explosion(Vec3f source)
{
    playthissound(source,"sounds/explosion.wav");
}

void blocked()
{
    playthissound(Vec3f(0,0,0),"sounds/blocked.wav");
}

void win()
{
    playthissound(Vec3f(0,0,0),"sounds/win.wav");
}

void lose()
{
    playthissound(Vec3f(0,0,0),"sounds/lose.wav");
}

void intro()
{
    playthissound( "sounds/babayetu.wav");
}

void march()
{
    playthissound( "sounds/march.wav");
}

void built()
{
    playthissound( "sounds/built.wav");
}
