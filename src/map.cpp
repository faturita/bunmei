#include <vector>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "openglutils.h"
#include "lodepng.h"
#include "usercontrols.h"
#include "math/yamathutil.h"
#include "font/DrawFonts.h"
#include "font/FontsBitmap.h"
#include "units/Unit.h"
#include "City.h"
#include "cityscreenui.h"
#include "Faction.h"
#include "resources.h"
#include "coordinator.h"
#include "map.h"

extern Controller controller;
extern Coordinator coordinator;

extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;

std::unordered_map<int, std::string> tiles;
Map map;
std::unordered_map<std::string, GLuint> maptextures;


extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;


int width = SCREEN_WIDTH;
int height = SCREEN_HEIGHT;
float mapzoom=1;

float cx;
float cy;

void zoommapin()
{
    if (mapzoom < 11)
        mapzoom=mapzoom*2;
}

void zoommapout()
{
    mapzoom=mapzoom/2;
}

void resetzoom()
{
    mapzoom = 2;
}

// Get Screen X,Y coordinate where the user clicked and convert it to opengl coordinates., X column, Y row
void centermap(int ccx, int ccy)  // lon,lat
{
    //1728,1117

    float fccx = ccx/(REAL_SCREEN_WIDTH*1.0)*width;
    float fccy = ccy/(REAL_SCREEN_HEIGHT*1.0)*height;

    ccx = fccx;
    ccy = fccy;

    // @FIXME: Parametrize all the resolution values.  This depends on the screen resolution, and the amount of clickable space
    int xsize = width/mapzoom;   //1200
    int ysize = height/mapzoom;    //800

    // Screen width and height come from OpenGL.
    cx = (int)(ccx*(xsize)/(float)width)+cx-xsize/2; 
    cy = (int)(ccy*(ysize)/(float)height)+cy-ysize/2;  

    printf("Center %f,%f\n",cx,cy);

    float dcx = (cx+8)-(width/2-MAPHALFWIDTH*16);
    float dcy = (cy+8)-(height/2-MAPHALFHEIGHT*16);

    printf("Center  adjustd %f,%f\n",dcx,dcy);

    int lon = (int)(dcx/16) - MAPHALFWIDTH;
    int lat = (int)(dcy/16) - MAPHALFHEIGHT;

    coordinate c = map.to_real(lat,lon);

    //printf("Location on the World Map (Lat,Lon)= (%d,%d)\n",c.lat,c.lon);
}

// Returns Real Lat,Lon of the current center of the screen (this can be compared to real locations of units and cities)
coordinate getCurrentCenter()
{
    //printf("Center %f,%f\n",cx,cy);

    float dcx = (cx+8)-(width/2-MAPHALFWIDTH*16);
    float dcy = (cy+8)-(height/2-MAPHALFHEIGHT*16);

    //printf("Center  adjustd %f,%f\n",dcx,dcy);

    int lon = (int)(dcx/16) - MAPHALFWIDTH;
    int lat = (int)(dcy/16) - MAPHALFHEIGHT;

    coordinate c = map.to_real(lat,lon);

    //printf("Location on the World Map (Lat,Lon)= (%d,%d)\n",c.lat,c.lon);    

    return c;
}

// Convert mouse coordinates into lat,lon map coordinates without any offset adjustment.
// So clicking on the center of the screen will give 0,0 result, and on the bottom right, +,+
coordinate convertToMap(int ccx, int ccy, int gridsize)
{
    float fccx = ccx/(REAL_SCREEN_WIDTH*1.0)*width;
    float fccy = ccy/(REAL_SCREEN_HEIGHT*1.0)*height;

    ccx = fccx;
    ccy = fccy; 

    float dcx = (ccx+gridsize/2)-(width/2-MAPHALFWIDTH*gridsize);
    float dcy = (ccy+gridsize/2)-(height/2-MAPHALFHEIGHT*gridsize);

    int lon = (int)(dcx/gridsize) - MAPHALFWIDTH;
    int lat = (int)(dcy/gridsize) - MAPHALFHEIGHT;

    //coordinate c = map.to_real(lat,lon);            // With offset
    coordinate c(lat,lon);                              // Without offset

    return c;
}

void centermapinmap(int lat, int lon)
{
    //printf("Location on the World Map (Lat,Lon)= (%d,%d)\n",lat,lon);

    cx = width/2.0 + lon*16.0;            // 0-1200
    cy = height/2.0 + lat*16.0 ;            // 0-800

    //printf("Center %f,%f\n",cx,cy);
}

void unfog(int lat, int lon)
{
    coordinate c = coordinate(lat,lon);
    for(int llat=-1;llat<=1;llat++)
        for (int llon=-1;llon<=1;llon++)
        {
            int lllat=c.lat+llat,lllon=c.lon+llon;
            map(lllat,lllon).visible = true;
        }
}


// These two functions perform the right mapping from model coordinates to opengl coordinates
void place(int x, int y, int sizex, int sizey, const char* modelName)
{
    placeMark((width/2)+x,-y,sizex,sizey,modelName);      // x,y x-> column y-> row  
}

void place(int x, int y, int sizex, int sizey, GLuint _texture)
{
    placeMark((width/2)+x,-y,sizex,sizey,_texture);      // x,y x-> column y-> row  
}

void placeFloat(float x, float y, int sizex, int sizey, GLuint _texture)
{
    placeMark((width/2)+x,-y,sizex,sizey,_texture);      // x,y x-> column y-> row  
}

// --------

void place(int x, int y, int size, const char* modelName)
{
    place(x,y,size,size,modelName);   // x,y x-> column y-> row  
}

void placeTile(int x, int y, const char* modelName)
{
    place(x*16,y*16,16,16,modelName);      // x,y x-> column y-> row  
}

void placeTile(int x, int y, int size, const char* modelName)
{
    place(x*16,y*16,size,size,modelName);      // x,y x-> column y-> row  
}

// Lat, lon are the real lat,lon coordinates of the map, and they will be converted to FIXED parameters on the screen.
void placeThisTile(int lat, int lon, int size, const char* filename)
{
    coordinate c = map.to_screen(lat,lon);
    place(c.lon*16,c.lat*16,size,size,filename);      // x,y x-> column y-> row  
}

void placeThisUnit(float flat, float flon, int size, const char* filename, int red, int green, int blue)
{
    char modelName[256];
    sprintf(modelName,"%s_%d_%d_%d", filename, red, green, blue);
    GLuint _texture = preloadUnitTexture(filename, modelName,red,green,blue);

    int lat = flat;
    int lon = flon;
    coordinate c = map.to_screen(lat,lon);

    flat = c.lat + (flat - lat);
    flon = c.lon + (flon - lon);

    placeFloat(flon*16,flat*16,size,size,_texture);      // x,y x-> column y-> row  
}

void placeThisCity(int lat, int lon, int red, int green, int blue)
{
    char modelName[256];
    sprintf(modelName,"%s_%d_%d_%d", "assets/assets/map/city_r.png_%d_%d_%d", red, green, blue);
    GLuint _texture = preloadCityTexture("assets/assets/map/city_r.png",modelName, red,green,blue);
    coordinate c = map.to_screen(lat,lon);
    place(c.lon*16,c.lat*16,16,16,_texture);      // x,y x-> column y-> row  
}

void drawGrid()
{
    place(0,0,16,16,"assets/assets/terrain/land.png");      // x,y x-> column y-> row  

    place(10*16,10*16, 16,16,"assets/assets/terrain/land.png");    

    place(-10*16,10*16, 16,16,"assets/assets/terrain/land.png");   

    place(10*16,-10*16, 16,16,"assets/assets/terrain/land.png");   

    place(-10*16,-10*16, 16,16,"assets/assets/terrain/land.png");    

    for(int col=-MAPHALFWIDTH;col<=MAPHALFWIDTH-1;col++)
        for (int row=-MAPHALFHEIGHT;row<=MAPHALFHEIGHT-1;row++)
        {
            place(col*16,row*16,16,16,"assets/assets/general/grid.png");      // x,y x-> column y-> row  
        }
}


void drawIntro()
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


    sprintf (str, "Bunmei: Craddle of civilization.");
    drawString(0,-700,1,str,0.1f);

    placeMark(600,-400,1000,"assets/art/opening.png");


    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
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

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glTranslatef(0, -(height/2), 1);

        glDepthMask(GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //drawGrid();

        // Water or Land
        for(int lat=map.minlat;lat<=map.maxlat-1;lat++)
        {
            for(int lon=map.minlon;lon<=map.maxlon-1;lon++)
            {
                if (map(lat,lon).visible) placeTile(lon,lat,tiles[map(lat,lon).code].c_str());
            }
        }

        // Biomas, including biomas on ocean (like river mouths)
        for(int lat=map.minlat;lat<=map.maxlat-1;lat++)
        {
            for(int lon=map.minlon;lon<=map.maxlon-1;lon++)
            {
                if (map(lat,lon).visible && map(lat,lon).bioma!=0) placeTile(lon,lat,tiles[map(lat,lon).bioma].c_str());
            }
        }

        // Coasts
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                int land = map(lat,lon).code;
                mapcell next = map.south(lat,lon);
                coordinate c = map.isouth(lat,lon);

                if (map(lat,lon).visible && land == 1 && next.code == 0)
                {
                    if ((map(lat,lon).bioma & 0xf2) != 0xa2) 
                    {
                        place(16*c.lon-4,   16*c.lat-8,8,8,"assets/assets/terrain/coast_s2.png");    
                        place(16*c.lon+4,   16*c.lat-8,8,8,"assets/assets/terrain/coast_s2.png"); 
                    }
                    else
                    {
                        place(16*c.lon-4,   16*c.lat-8,8,8,"assets/assets/terrain/coast_sm1.png");    
                        place(16*c.lon+4,   16*c.lat-8,8,8,"assets/assets/terrain/coast_sm2.png"); 
                    }
                }


                next = map.north(lat,lon);
                c = map.inorth(lat,lon);

                if (map(lat,lon).visible && land == 1 && next.code == 0)
                {
                    if ((map(lat,lon).bioma & 0xf8) != 0xa8)
                    {
                        place(16*c.lon-4,   16*c.lat+8,8,8,"assets/assets/terrain/coast_n2.png");    
                        place(16*c.lon+4,   16*c.lat+8,8,8,"assets/assets/terrain/coast_n2.png"); 
                    }
                    else
                    {
                        place(16*c.lon-4,   16*c.lat+8,8,8,"assets/assets/terrain/coast_nm2.png");    
                        place(16*c.lon+4,   16*c.lat+8,8,8,"assets/assets/terrain/coast_nm1.png"); 
                    }
                }

                next = map.east(lat,lon);
                c = map.ieast(lat,lon);

                if (map(lat,lon).visible && land == 1 && next.code == 0)
                {
                    if ((map(lat,lon).bioma & 0xf4) != 0xa4)
                    {
                        place(16*c.lon-8,   16*c.lat-4,8,8,"assets/assets/terrain/coast_e5.png");    
                        place(16*c.lon-8,   16*c.lat+4,8,8,"assets/assets/terrain/coast_e5.png"); 
                    }
                    else
                    {
                        place(16*c.lon-8,   16*c.lat-4,8,8,"assets/assets/terrain/coast_em2.png");    
                        place(16*c.lon-8,   16*c.lat+4,8,8,"assets/assets/terrain/coast_em1.png"); 
                    }
                }

                next = map.west(lat,lon);
                c = map.iwest(lat,lon);

                if (map(lat,lon).visible && land == 1 && next.code == 0)
                {
                    if ((map(lat,lon).bioma & 0xf1) != 0xa1) 
                    {
                        place(16*c.lon+8,   16*c.lat-4,8,8,"assets/assets/terrain/coast_w2.png");    
                        place(16*c.lon+8,   16*c.lat+4,8,8,"assets/assets/terrain/coast_w2.png"); 
                    } 
                    else 
                    {
                        place(16*c.lon+8,   16*c.lat-4,8,8,"assets/assets/terrain/coast_wm1.png");    
                        place(16*c.lon+8,   16*c.lat+4,8,8,"assets/assets/terrain/coast_wm2.png"); 
                    }
                }
            }

        // Add Resources
        for(int lat=map.minlat;lat<map.maxlat;lat++)
        {
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                int size = 15;
                if (map(lat,lon).resource == 0x10d) size = 7;   // Some resources are smaller in how they are represented in the map
                if (map(lat,lon).visible && map(lat,lon).bioma!=0 && map(lat,lon).resource > 0) placeTile(lon,lat,size,tiles[map(lat,lon).resource].c_str());
            }
        }

        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).visible)
                {
                    if (!(map.south(lat,lon).visible)) placeTile(lon,lat,16,"assets/assets/map/fog_s.png");
                    if (!(map.north(lat,lon).visible)) placeTile(lon,lat,16,"assets/assets/map/fog_n.png");
                    if (!(map.west(lat,lon).visible)) placeTile(lon,lat,16,"assets/assets/map/fog_w.png");
                    if (!(map.east(lat,lon).visible)) placeTile(lon,lat,16,"assets/assets/map/fog_e.png");

                }
            }

        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (controller.showLandOwnership)
                if (!map(lat,lon).isFreeLand())
                {
                    int faction = map(lat,lon).getOwnedBy();
                    int red = factions[faction]->red;
                    int green = factions[faction]->green;
                    int blue = factions[faction]->blue;

                    placeThisUnit(lat,lon,16,"assets/assets/general/owned.png",red,green,blue);
                }
            }

        drawUnitsAndCities();

        openCityScreen();   


    } glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void drawUnitsAndCities()
{
    static int count=0;

    for (auto& [k,u] : units) 
    {
        coordinate c = map.to_screen(u->latitude,u->longitude);
        unfog(c.lat,c.lon);

        // @NOTE: Show the units that are not currently being controlled, except if they are in the same lat,lon.
        if (coordinator.a_u_id != u->id && (units.find(coordinator.a_u_id)==units.end() || units[coordinator.a_u_id]->getCoordinate() != u->getCoordinate()))
            u->draw();
    }

    for (auto& [k, c] : cities) 
    {
        coordinate co = map.to_screen(c->latitude,c->longitude);
        if (map(co.lat,co.lon).visible)
        {
            c->draw();
        }
    }

    // Draw last the unit that you want on top of the stack (selected unit)
    for (auto& [k,u] : units) 
    {
        if (u->faction == coordinator.a_f_id)
        {
            if (coordinator.a_u_id== u->id)
            {
                if (!u->movementCompleted() || u->isFortified())
                    u->draw();
                else
                if (count++ % factions[coordinator.a_f_id]->blinkingrate < (factions[coordinator.a_f_id]->blinkingrate/2))
                {
                    u->draw();
                }
            }
        }
    }

    if (factions[coordinator.a_f_id]->blinkingrate < 70) factions[coordinator.a_f_id]->blinkingrate++;

    map.setCenter(factions[coordinator.a_f_id]->vmapoffset,factions[coordinator.a_f_id]->mapoffset);
}


void openCityScreen()
{
    if (controller.view == 2)
    {
        City *city = cities[controller.cityid];
        coordinate co = getCurrentCenter();
        coordinate c = map.to_screen(city->latitude,city->longitude);
        drawCityScreen(c.lat,c.lon,city);
    }       
}

