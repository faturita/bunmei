#ifndef MAP_H
#define MAP_H

#include "math/yamathutil.h"

struct mapcell
{
    mapcell(int code)
    {
        this->code = code;
        this->visible = false;      // make it a vector per faction
        this->bioma = 0;// By default, nothing
        this->resource = 0;
    }

    int code;
    bool visible;
    int bioma;
    int resource;
};

struct coordinate
{
    coordinate(int lat,int lon)
    {
        this->lat = lat;
        this->lon = lon;
    }
    int lat;
    int lon;
};

class Map
{
    private:
        std::vector<std::vector<mapcell>> map;


    public:
        int centerx, centery;
        int minlat = -20;
        int maxlat = 20;
        int minlon = -35;
        int maxlon = 35;

        void init()
        {
            centerx = 0;
            centery = 0;
            int no_of_cols = maxlon-minlon;
            int no_of_rows = maxlat-minlat;
            int initial_value = 0;

            map.resize(no_of_rows, std::vector<mapcell>(no_of_cols, initial_value));
        }

        void setCenter(int x, int y)
        {
            centerx = x;
            centery = y;
        }

        mapcell &operator()(int lat, int lon)
        {
            int x=lat+abs(minlat)+centerx,y=lon+abs(minlon)+centery;
            x = rotclipped(x,0,maxlat-minlat-1);
            y = rotclipped(y,0,maxlon-minlon-1);
            return map[x][y];
        }

        mapcell &set(int lat, int lon)
        {
            int x=lat+abs(minlat),y=lon+abs(minlon);
            x = clipped(x,0,maxlat-minlat-1);
            y = clipped(y,0,maxlon-minlon-1);
            return map[x][y];
        }

        mapcell &operator()(coordinate c)
        {
            return operator()(c.lat,c.lon);
        }

        mapcell &south(int lat, int lon)
        {
            return operator()(lat-1,lon);
        }

        mapcell &north(int lat, int lon)
        {
            return operator()(lat+1,lon);
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
            return coordinate(lat-1,lon);
        }

        coordinate inorth(int lat, int lon)
        {
            return coordinate(lat+1,lon);
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
            int x=lat+abs(minlat)+centerx,y=lon+abs(minlon)+centery;
            x = rotclipped(x,0,maxlat-minlat-1);
            y = rotclipped(y,0,maxlon-minlon-1);
            return coordinate(x,y);
        }

        coordinate remap(int lat, int lon)
        {
            lat = lat - centerx;
            lon = lon - centery;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        coordinate mapre(int lat, int lon)
        {
            lat = lat + centerx;
            lon = lon + centery;

            return coordinate(lat,lon);
        }

};


void zoommapin();

void zoommapout();

void centermap(int ccx, int ccy);
void centermapinmap(int latitude, int longitude);

void drawMap();
void initMap();

void placeInMap(int latitude, int longitude, int size, const char *filename);
void placeCityInMap(int lat, int lon, int size, const char* texture);


#endif // MAP_H
