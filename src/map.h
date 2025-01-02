#ifndef MAP_H
#define MAP_H

#include "openglutils.h"
#include "math/yamathutil.h"
#include "coordinate.h"


#define MAPHALFWIDTH 36
#define MAPHALFHEIGHT 24

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800

struct mapcell
{
    mapcell(int code)
    {
        this->code = code;
        this->visible = true;      // make it a vector per faction
        this->bioma = 0;// By default, nothing
        this->resource = 0;  // This is a special resource that can be obtained from the map.
    }

    int code;
    bool visible;
    int bioma;
    int resource;

    std::vector<int> resource_production_rate;   // List of resource production per tile.
};

class Map
{
    private:
        std::vector<std::vector<mapcell>> map;


    public:
        int offsetlat, offsetlon;
        int minlat = -MAPHALFHEIGHT;
        int maxlat = MAPHALFHEIGHT;
        int minlon = -MAPHALFWIDTH;
        int maxlon = MAPHALFWIDTH;

        void init()
        {
            offsetlat = 0;
            offsetlon = 0;
            int no_of_cols = maxlon-minlon;
            int no_of_rows = maxlat-minlat;
            int initial_value = 0;

            map.resize(no_of_rows, std::vector<mapcell>(no_of_cols, initial_value));
        }

        void setCenter(int lat, int lon)
        {
            offsetlat = lat;
            offsetlon = lon;
        }

        mapcell &operator()(int lat, int lon)
        {
            int row=lat+abs(minlat)+offsetlat,col=lon+abs(minlon)+offsetlon;
            row = rotclipped(row,0,maxlat-minlat-1);
            col = rotclipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }

        mapcell &set(int lat, int lon)
        {
            int row=lat+abs(minlat),col=lon+abs(minlon);
            row = clipped(row,0,maxlat-minlat-1);
            col = clipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }

        mapcell &operator()(coordinate c)
        {
            return operator()(c.lat,c.lon);
        }

        mapcell &south(int lat, int lon)
        {
            return operator()(lat+1,lon);
        }

        mapcell &north(int lat, int lon)
        {
            return operator()(lat-1,lon);
        }

        mapcell &west(int lat, int lon)
        {
            return operator()(lat,lon-1);
        }

        mapcell &east(int lat, int lon)
        {
            return operator()(lat,lon+1);
        }

        coordinate isouth(int lat, int lon)
        {
            return coordinate(lat+1,lon);
        }

        coordinate inorth(int lat, int lon)
        {
            return coordinate(lat-1,lon);
        }

        coordinate iwest(int lat, int lon)
        {
            return coordinate(lat,lon-1);
        }

        coordinate ieast(int lat, int lon)
        {
            return coordinate(lat,lon+1);
        }

        coordinate i(int lat, int lon)
        {
            int row=lat+abs(minlat)+offsetlat,col=lon+abs(minlon)+offsetlon;
            row = rotclipped(row,0,maxlat-minlat-1);
            col = rotclipped(col,0,maxlon-minlon-1);
            return coordinate(row,col);
        }

        // Convert offset lat,lon (zero,zero can be shifted) to screen fixed lat,lon (zero,zero is the center of the screen)
        coordinate to_fixed(int lat, int lon)
        {
            lat = lat - offsetlat;
            lon = lon - offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        // Convert fixed lat, lon (zero,zero is the center of the screen) to offset lat, lon (zero,zero can be shifted)
        coordinate to_offset(int lat, int lon)
        {
            lat = lat + offsetlat;
            lon = lon + offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

};

void drawMap();

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
void adjustMovements();
void openCityScreen();

void place(int x, int y, int sizex, int sizey, const char* modelName);
void place(int x, int y, int sizex, int sizey, GLuint _texture);
void place(int x, int y, int size, const char* modelName);
void placeTile(int x, int y, const char* modelName);
void placeTile(int x, int y, int size, const char* modelName);
void placeThisUnit(int lat, int lon, int size, const char* modelName, int red=255, int green=0, int blue=0);
void placeThisCity(int lat, int lon, int red, int green, int blue);

#endif   // MAP_H