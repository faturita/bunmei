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
#include "ui.h"
#include "map.h"

std::unordered_map<std::string, GLuint> maptextures;
extern std::unordered_map<int, std::string> tiles;
extern Map map;

int width = 1200;
int height = 800;
int mapzoom=1;

float cx;
float cy;

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


// Get Screen X,Y coordinate where the user clicked and convert it to opengl coordinates., X column, Y row
void centermap(int ccx, int ccy)  // lon,lat
{
    //1728,1117

    float fccx = ccx/1728.0*width;
    float fccy = ccy/1117.0*height;

    ccx = fccx;
    ccy = fccy;

    // @FIXME: Parametrize all the resolution values.  This depends on the screen resolution, and the amount of clickable space
    int xsize = width/mapzoom;   //1200
    int ysize = height/mapzoom;    //800

    // Screen width and height come from OpenGL.
    cx = (int)(ccx*(xsize)/(float)width)+cx-xsize/2; 
    cy = (int)(ccy*(ysize)/(float)height)+cy-ysize/2;  

    printf("Center %f,%f\n",cx,cy);

    float dcx = (cx+8)-(width/2-35*16);
    float dcy = (cy+8)-(height/2-20*16);

    printf("Center  adjustd %f,%f\n",dcx,dcy);

    int lon = (int)(dcx/16) - 35;
    int lat = (int)(dcy/16) - 20;

    coordinate c = map.to_offset(lat,lon);

    //printf("Location on the World Map (Lat,Lon)= (%d,%d)\n",c.lat,c.lon);
}

// Returns Real Lat,Lon of the current center of the screen (this can be compared to real locations of units and cities)
coordinate getCurrentCenter()
{
    //printf("Center %f,%f\n",cx,cy);

    float dcx = (cx+8)-(width/2-35*16);
    float dcy = (cy+8)-(height/2-20*16);

    //printf("Center  adjustd %f,%f\n",dcx,dcy);

    int lon = (int)(dcx/16) - 35;
    int lat = (int)(dcy/16) - 20;

    coordinate c = map.to_offset(lat,lon);

    //printf("Location on the World Map (Lat,Lon)= (%d,%d)\n",c.lat,c.lon);    

    return c;
}

// Convert mouse coordinates into lat,lon map coordinates without any offset adjustment.
// So clicking on the center of the screen will give 0,0 result, and on the bottom right, +,+
coordinate convertToMap(int ccx, int ccy, int gridsize)
{
    float fccx = ccx/1728.0*width;
    float fccy = ccy/1117.0*height;

    ccx = fccx;
    ccy = fccy; 

    float dcx = (ccx+16)-(width/2-35*gridsize);
    float dcy = (ccy+16)-(height/2-20*gridsize);

    int lon = (int)(dcx/gridsize) - 35;
    int lat = (int)(dcy/gridsize) - 20;

    //coordinate c = map.to_offset(lat,lon);            // With offset
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

void placeThisUnit(int lat, int lon, int size, const char* filename, int red, int green, int blue)
{
    char modelName[256];
    sprintf(modelName,"%s_%d_%d_%d", filename, red, green, blue);
    GLuint _texture = preloadUnitTexture(filename, modelName,red,green,blue);
    coordinate c = map.to_fixed(lat,lon);
    place(c.lon*16,c.lat*16,size,size,_texture);      // x,y x-> column y-> row  
}

void placeThisCity(int lat, int lon, int red, int green, int blue)
{
    char modelName[256];
    sprintf(modelName,"%s_%d_%d_%d", "assets/assets/map/city_r.png_%d_%d_%d", red, green, blue);
    GLuint _texture = preloadCityTexture("assets/assets/map/city_r.png",modelName, red,green,blue);
    coordinate c = map.to_fixed(lat,lon);
    place(c.lon*16,c.lat*16,16,16,_texture);      // x,y x-> column y-> row  
}

void drawGrid()
{
    place(0,0,16,16,"assets/assets/terrain/land.png");      // x,y x-> column y-> row  

    place(10*16,10*16, 16,16,"assets/assets/terrain/land.png");    

    place(-10*16,10*16, 16,16,"assets/assets/terrain/land.png");   

    place(10*16,-10*16, 16,16,"assets/assets/terrain/land.png");   

    place(-10*16,-10*16, 16,16,"assets/assets/terrain/land.png");    

    for(int col=-35;col<=34;col++)
        for (int row=-20;row<=19;row++)
        {
            place(col*16,row*16,16,16,"assets/assets/general/grid.png");      // x,y x-> column y-> row  
        }
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
                        place(16*c.lon-8,   16*c.lat-4,8,8,"assets/assets/terrain/coast_em1.png");    
                        place(16*c.lon-8,   16*c.lat+4,8,8,"assets/assets/terrain/coast_em2.png"); 
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
                        place(16*c.lon+8,   16*c.lat-4,8,8,"assets/assets/terrain/coast_wm2.png");    
                        place(16*c.lon+8,   16*c.lat+4,8,8,"assets/assets/terrain/coast_wm1.png"); 
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

        drawUnitsAndCities();

        adjustMovements();

        openCityScreen();   


    } glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

// void drddawMap()
// {
//     // This will make things dark.

//     glMatrixMode(GL_PROJECTION);
//     glPushMatrix();
//     glLoadIdentity();

//     int xsize = width/mapzoom;
//     int ysize = height/mapzoom;

//     if (mapzoom==1)
//     {
//         cx = width/2;
//         cy = height/2;
//     }


//     glOrtho(cx-xsize/2, cx+xsize/2, cy+ysize/2, cy-ysize/2, -1, 1);
//     glMatrixMode(GL_MODELVIEW);
//     glPushMatrix();
//     glLoadIdentity();
//     glColor4f(1.0f, 1.0f, 1.0f, 1);
//     glDisable(GL_DEPTH_TEST);
//     glRotatef(180.0f,0,0,1);
//     glRotatef(180.0f,0,1,0);



//     glClearColor(0.0f,0.0f,0.0f,1.0f);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//     //char str[256];
//     //sprintf (str, "Civilization");
//     // width, height, 0 0 upper left
//     //drawString(0,-30,1,str,0.2f,1.0f,1.0f,1.0f);

//     glMatrixMode(GL_MODELVIEW);
//     glPushMatrix(); {
//         glTranslatef(0, -400, 1);

//         glDepthMask(GL_FALSE);

//         glEnable(GL_BLEND);
//         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//         // Water or Land
//         for(int lat=map.minlat;lat<map.maxlat;lat++)
//         {
//             for(int lon=map.minlon;lon<map.maxlon;lon++)
//             {
//                 if (map(lat,lon).visible) place(lon,lat,16,tiles[map(lat,lon).code].c_str());
//             }
//         }

//         // Biomas, including biomas on ocean (like river mouths)
//         for(int lat=map.minlat;lat<map.maxlat;lat++)
//         {
//             for(int lon=map.minlon;lon<map.maxlon;lon++)
//             {
//                 if (map(lat,lon).visible && map(lat,lon).bioma!=0) place(lon,lat,16,tiles[map(lat,lon).bioma].c_str());
//             }
//         }

//         // Coasts
//         for(int lat=map.minlat;lat<map.maxlat;lat++)
//             for(int lon=map.minlon;lon<map.maxlon;lon++)
//             {
//                 int land = map(lat,lon).code;
//                 mapcell next = map.south(lat,lon);
//                 coordinate c = map.isouth(lat,lon);

//                 if (map(lat,lon).visible && land == 1 && next.code == 0)
//                 {
//                     if ((map(lat,lon).bioma & 0xf2) != 0xa2) 
//                     {
//                         placeMark(600+16*c.lon-4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_s2.png");
//                         placeMark(600+16*c.lon+4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_s2.png");
//                     }
//                     else
//                     {
//                         placeMark(600+16*c.lon-4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_sm1.png");
//                         placeMark(600+16*c.lon+4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_sm2.png");
//                     }
//                 }


//                 next = map.north(lat,lon);
//                 c = map.inorth(lat,lon);

//                 if (map(lat,lon).visible && land == 1 && next.code == 0)
//                 {
//                     if ((map(lat,lon).bioma & 0xf8) != 0xa8)
//                     {
//                         placeMark(600+16*c.lon-4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_n2.png");
//                         placeMark(600+16*c.lon+4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_n2.png");
//                     }
//                     else
//                     {
//                         placeMark(600+16*c.lon-4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_nm2.png");
//                         placeMark(600+16*c.lon+4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_nm1.png");
//                     }
//                 }

//                 next = map.east(lat,lon);
//                 c = map.ieast(lat,lon);

//                 if (map(lat,lon).visible && land == 1 && next.code == 0)
//                 {
//                     if ((map(lat,lon).bioma & 0xf4) != 0xa4)
//                     {
//                         placeMark(600+16*c.lon-8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_e5.png");
//                         placeMark(600+16*c.lon-8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_e5.png");
//                     }
//                     else
//                     {
//                         placeMark(600+16*c.lon-8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_em1.png");
//                         placeMark(600+16*c.lon-8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_em2.png");
//                     }
//                 }

//                 next = map.west(lat,lon);
//                 c = map.iwest(lat,lon);

//                 if (map(lat,lon).visible && land == 1 && next.code == 0)
//                 {
//                     if ((map(lat,lon).bioma & 0xf1) != 0xa1) 
//                     {
//                         placeMark(600+16*c.lon+8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_w2.png");
//                         placeMark(600+16*c.lon+8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_w2.png");
//                     } 
//                     else 
//                     {
//                         placeMark(600+16*c.lon+8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_wm2.png");
//                         placeMark(600+16*c.lon+8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_wm1.png");
//                     }
//                 }
//             }

//         // Add Resources
//         for(int lat=map.minlat;lat<map.maxlat;lat++)
//         {
//             for(int lon=map.minlon;lon<map.maxlon;lon++)
//             {
//                 int size = 15;
//                 if (map(lat,lon).resource == 0x10d) size = 7;   // Some resources are smaller in how they are represented in the map
//                 if (map(lat,lon).visible && map(lat,lon).bioma!=0 && map(lat,lon).resource > 0) place(lon,lat,size,tiles[map(lat,lon).resource].c_str());
//             }
//         }

//         static int count=0;



//         for(int lat=map.minlat;lat<map.maxlat;lat++)
//             for(int lon=map.minlon;lon<map.maxlon;lon++)
//             {
//                 if (map(lat,lon).visible)
//                 {
//                     if (!(map.south(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_s.png");
//                     if (!(map.north(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_n.png");
//                     if (!(map.west(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_w.png");
//                     if (!(map.east(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_e.png");

//                 }
//             }


//         if (controller.registers.pitch!=0 || controller.registers.roll !=0)
//         {
//             // Receives real latitude and longitude (contained in the unit)
//             int lon = units[controller.controllingid]->longitude;
//             int lat = units[controller.controllingid]->latitude;

//             // Convert latitude and longitude into remaped coordinates
//             coordinate c = map.to_fixed(lat,lon);

//             lon = c.lon;
//             lat = c.lat;

//             int val = lat;
//             val = ((int)val+controller.registers.pitch);
//             lat = clipped(val,map.minlat,map.maxlat-1);

//             lon = lon + controller.registers.roll;

//             if (val>=map.maxlat) {
//                 lon=lon*(-1);
//                 lat=map.maxlat-1;
//             }

//             if ((val-lat)<0) {
//                 lon=lon*(-1);
//                 lat=map.minlat;
//             }

//             if (units[controller.controllingid]->availablemoves>0)
//             {
//                 if (map(lat,lon).code==1)
//                 {

//                     coordinate c = map.to_offset(lat,lon);

//                     // Confirm the change if it is moving into land.
//                     units[controller.controllingid]->latitude = c.lat;
//                     units[controller.controllingid]->longitude = c.lon; 

//                     units[controller.controllingid]->availablemoves--;
//                 }
//             } 


//             if (units[controller.controllingid]->availablemoves==0)
//             {
//                 if (units.size()>controller.controllingid+1)
//                     controller.controllingid++;  
//                 else
//                     controller.endofturn = true; 
//             }


//             controller.registers.pitch= controller.registers.roll = 0;     

//             printf("Lat %d Lon %d   Land %d  Bioma  %d  \n",units[controller.controllingid]->latitude,units[controller.controllingid]->longitude, map(lat,lon).code, map(lat,lon).bioma);   
//         }


//         // if (controller.registers.pitch!=0 || controller.registers.roll !=0)
//         // {
//         //     int val = units[controller.controllingid]->latitude;
            
//         //     val = ((int)val+controller.registers.pitch);
//         //     int lat = clipped(val,map.minlat,map.maxlat-1);

//         //     int lon = units[controller.controllingid]->longitude + controller.registers.roll;

//         //     if (val>=map.maxlat) {
//         //         lon=lon*(-1);
//         //         lat=map.maxlat-1;
//         //     }

//         //     if ((val-lat)<0) {
//         //         lon=lon*(-1);
//         //         lat=map.minlat;
//         //     }


//         //     lon = ((int)lon) + abs(map.minlon) + map.centerx;
//         //     lon = lon % (map.maxlon-map.minlon);
//         //     if (lon<0) lon = (map.maxlon-map.minlon) + lon;
//         //     lon = lon - abs(map.minlon);  

//         //     if (units[controller.controllingid]->availablemoves>0)
//         //     {
//         //         if (map(lat,lon).code==1)
//         //         {
//         //             // Confirm the change if it is moving into land.
//         //             units[controller.controllingid]->latitude = lat;
//         //             units[controller.controllingid]->longitude = lon;  

//         //             units[controller.controllingid]->availablemoves--;
//         //         }
//         //     } 


//         //     if (units[controller.controllingid]->availablemoves==0)
//         //     {
//         //         if (units.size()>controller.controllingid+1)
//         //             controller.controllingid++;  
//         //         else
//         //             controller.endofturn = true; 
//         //     }


//         //     controller.registers.pitch= controller.registers.roll = 0;     

//         //     printf("Lat %d Lon %d   Land %d  Bioma  %d  \n",lat,lon, map(lat,lon).code, map(lat,lon).bioma);   
//         // }

//         /** 
//         if (count++ % 100 < 50)
//         {
//             //int lat = ((int)controller.registers.pitch) + abs(map.minlat) - map.centerx;
//             //lat = lat % (map.maxlat-map.minlat);
//             //if (lat<0) lat = (map.maxlat-map.minlat) + lat;
//             //lat = lat - abs(map.minlat);

//             int val = ((int)controller.registers.pitch);
//             int lat = clipped(val,map.minlat,map.maxlat-1);

//             if (val>=map.maxlat) {
//                 controller.registers.roll=controller.registers.roll*(-1);
//                 controller.registers.pitch=map.maxlat-1;
//             }

//             if ((val-lat)<0) {
//                 controller.registers.roll=controller.registers.roll*(-1);
//                 controller.registers.pitch=map.minlat;
//             }


//             int lon = ((int)controller.registers.roll) + abs(map.minlon) - map.centery;
//             lon = lon % (map.maxlon-map.minlon);
//             if (lon<0) lon = (map.maxlon-map.minlon) + lon;
//             lon = lon - abs(map.minlon);

//             placeThisUnit(lon,lat,16,"assets/assets/units/settlers.png");

//             printf("Lat %d Lon %d   Land %d  Bioma  %d  \n",lat,lon, map(lat,lon).code, map(lat,lon).bioma);
//         }*/

//         pop = 0;
//         for (auto& c : cities) 
//         {
//             coordinate co = map.to_fixed(c->latitude,c->longitude);
//             if (map(co.lat,co.lon).visible)
//             {
//                 c->draw();
//             }
//             pop += c->pop;
//         }

//         for (auto& u : units) 
//         {
//             if (u->faction == controller.faction)
//             {
//                 coordinate c = map.to_fixed(u->latitude,u->longitude);
//                 unfog(c.lat,c.lon);
//                 if (controller.controllingid != u->id)
//                     u->draw();
//             }
//         }

//         // Draw last the unit that you want on top of the stack (selected unit)
//         for (auto& u : units) 
//         {
//             if (u->faction == controller.faction)
//             {
//                 if (controller.controllingid == u->id)
//                 {
//                     if (count++ % 70 < 35)
//                     {
//                         u->draw();
//                     }
//                 }
//             }
//         }

//         map.setCenter(0,controller.registers.yaw);


//         if (controller.view == 2)
//         {
//             coordinate co = getcenteredlatlon();
//             drawCityScreen(co.lat,co.lon);
//         }

//         //placeMark(600,0,145,137,"assets/assets/general/font1.png");

//         //drawBlueBox(600,0,100,100);

//         //placeLetter(600,0,"?");

//         //drawRedBox(600,0,30,10);

//         //placeWord(600,0,"EXIT");

//         //placeMark(600,0,8,16,"assets/assets/general/34.png");

//         glDisable(GL_BLEND);

//     } glPopMatrix();

//     glEnable(GL_DEPTH_TEST);
//     glPopMatrix();
//     glMatrixMode(GL_PROJECTION);
//     glPopMatrix();

// }
