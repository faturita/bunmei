#ifndef MAPMODEL_H
#define MAPMODEL_H

#include "math/yamathutil.h"
#include "coordinate.h"

// Map dimension presets.  Each mapsize covers the whole screen exactly at its default zoom:
// zoom level N means the N-th zoom out step, which is mapzoom = 2^-(N-1) internally.
struct MapDimension
{
    int halfwidth;
    int halfheight;
    float defaultzoom;      // mapzoom value at which the whole map covers the screen.
};

static const MapDimension MAPDIMENSIONS[] = {
    {36,  24,  1.0f},       // mapsize 1:   72x48 covers the screen at zoom level 1 (mapzoom 1)
    {72,  48,  0.5f},       // mapsize 2:  144x96 covers the screen at zoom level 2 (mapzoom 0.5)
    {144, 96,  0.25f},      // mapsize 3: 288x192 covers the screen at zoom level 3 (mapzoom 0.25)
    {288, 192, 0.125f},     // mapsize 4: 576x384 covers the screen at zoom level 4 (mapzoom 0.125)
    {576, 384, 0.0625f}     // mapsize 5: 1152x768 covers the screen at zoom level 5 (mapzoom 0.0625)
};

#define NUMBER_OF_MAPSIZES  ((int)(sizeof(MAPDIMENSIONS)/sizeof(MAPDIMENSIONS[0])))
#define DEFAULT_MAPSIZE 1

inline MapDimension getMapDimension(int mapsize)
{
    if (mapsize < 1) mapsize = 1;
    if (mapsize > NUMBER_OF_MAPSIZES) mapsize = NUMBER_OF_MAPSIZES;
    return MAPDIMENSIONS[mapsize-1];
}

#define MAPHALFWIDTH  (MAPDIMENSIONS[DEFAULT_MAPSIZE-1].halfwidth)
#define MAPHALFHEIGHT (MAPDIMENSIONS[DEFAULT_MAPSIZE-1].halfheight)

#define UNASSIGNED_LAND -1
#define FREE_LAND       -1

struct mapcell
{
    public:
    int c_id_owner = UNASSIGNED_LAND;                           // Free land.
    int f_id_owner = FREE_LAND;                                 // Free land.

    int owners = 0;


    mapcell(int code)
    {
        this->code = code;
        this->visible = true;      // make it a vector per faction
        this->bioma = 0;// By default, nothing
        this->resource = 0;  // This is a special resource that can be obtained from the map.
        this->improvements = 0;  // Improvements bitmap.  Each bit represents a different improvement.  For instance, bit 0 is road, bit 1 is irrigation, etc.
    }

    int code;
    bool visible;
    int bioma;
    int resource;
    int improvements;

    std::vector<int> resource_production_rate;   // List of resource production per tile.

    bool belongsToCity()
    {
        return c_id_owner != UNASSIGNED_LAND;
    }

    bool belongsToCity(int f_id, int c_id)
    {
        return (f_id_owner == f_id && c_id_owner == c_id );    
    }

    void setCityOwnership(int f_id, int c_id)
    {
        c_id_owner = c_id;
        setOwnedBy(f_id);
    }

    void releaseCityOwnership()
    {
        c_id_owner = UNASSIGNED_LAND;
        releaseOwner();
    }

    bool isOccupied(int f_id, int c_id)
    {
        return  (   (f_id_owner != FREE_LAND        && f_id_owner != f_id) ||
                    (c_id_owner != UNASSIGNED_LAND  && c_id_owner != c_id) );
    }

    bool isUnassignedLand()
    {
        return c_id_owner == UNASSIGNED_LAND;
    }

    bool isFreeLand()
    {
        return f_id_owner == FREE_LAND;
    }

    void releaseOwner()
    {
        if (owners==1) 
        {
            f_id_owner = FREE_LAND;
            owners = 0;
        }
        else if (owners>1)
            owners--;
    }

    bool isOwnedBy(int f_id)
    {
        return f_id_owner == f_id;
    }

    int getOwnedBy()
    {
        return f_id_owner;
    }

    void setOwnedBy(int f_id)
    {
        if (f_id_owner == f_id) 
        {
            owners++;
        }
        else
        {
            f_id_owner = f_id;
            owners = 1;
        }
    }

    void buildIrrigation()
    {
        improvements |= 0x01;
    }

    void buildMine()
    {
        improvements |= 0x02;
    }

    void buildRoad()
    {
        improvements |= 0x04;
    }
};

class MapCylindrical
{
    private:
        std::vector<std::vector<mapcell>> map;


    public:
        int offsetlat, offsetlon;
        int minlat;
        int maxlat;
        int minlon;
        int maxlon;

        void init(int halfheight, int halfwidth)
        {
            minlat = -halfheight;
            maxlat = halfheight;
            minlon = -halfwidth;
            maxlon = halfwidth;

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

        // Get the map cell at the SCREEN coordinates lat, lon.  For instance (zero,zero) is what should be at the center of the screen.
        mapcell &operator()(int lat, int lon)
        {
            int row=lat+abs(minlat)+offsetlat,col=lon+abs(minlon)+offsetlon;
            row = rotclipped(row,0,maxlat-minlat-1);
            col = rotclipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }


        // Set/Get the value of the map cell at the given lat,lon using lat,lon arithmetics without screen OFFSET.
        //   (you can use this function when you can have lat,lon arithmetics that span the spheroid map)
        mapcell &peek(int lat, int lon)
        {
            int row=lat+abs(minlat),col=lon+abs(minlon);
            row = rotclipped(row,0,maxlat-minlat-1);
            col = rotclipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }


        // Set/Get the value of the map cell at the given lat,lon using REAL lat,lon (of the map data structure)
        //    These are CLIPPED, so if you move away from the boundaries, you are affecting the last value.
        //    LAT,LON need always to be clipped.
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

        // Convert offset lat,lon (zero,zero can be shifted) to screen fixed lat,lon (zero,zero is the center of the screen)
        coordinate to_screen(int lat, int lon)
        {
            lat = lat - offsetlat;
            lon = lon - offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            lat += abs(minlat);

            lat = rotclipped(lat,0,maxlat-minlat-1);

            lat -= abs(minlat);

            return coordinate(lat,lon);
        }

        // Convert SCREEN lat,lon (zero,zero is the center of the screen) to REAL lat,lon
        coordinate to_real(int lat, int lon)
        {
            lat = lat + offsetlat;
            lon = lon + offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            lat += abs(minlat);

            lat = rotclipped(lat,0,maxlat-minlat-1);

            lat -= abs(minlat);

            return coordinate(lat,lon);
        }

        // Convert SCREEN lat,lon (zero,zero is the center of the screen) to REAL lat,lon
        coordinate to_real_without_offset(int lat, int lon)
        {
            lat = lat;
            lon = lon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);


            lat += abs(minlat);

            lat = rotclipped(lat,0,maxlat-minlat-1);

            lat -= abs(minlat);

            return coordinate(lat,lon);
        }

        coordinate to_real_without_offset(coordinate c)
        {
            return to_real_without_offset(c.lat,c.lon);
        }

        coordinate displacement(int lat, int lon, int pitch, int roll)
        {
            int row=lat+pitch,col=lon+roll;
            row = rotclipped(row,0,maxlat-minlat-1);
            col = rotclipped(col,0,maxlon-minlon-1);

            return coordinate(row,col);       
        }

};

class MapSpheroid
{
    private:
        std::vector<std::vector<mapcell>> map;


    public:
        int offsetlat, offsetlon;
        int minlat;
        int maxlat;
        int minlon;
        int maxlon;

        void init(int halfheight, int halfwidth)
        {
            minlat = -halfheight;
            maxlat = halfheight;
            minlon = -halfwidth;
            maxlon = halfwidth;

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

        // Get the map cell at the SCREEN coordinates lat, lon.  For instance (zero,zero) is what should be at the center of the screen.
        mapcell &operator()(int lat, int lon)
        {
            int val = lat+offsetlat;
            val = ((int)val+0);
            lat = clipped(val,minlat,maxlat-1);

            lon = lon + 0 + offsetlon;

            if (val>=maxlat) {
                lon=lon*(-1)-1;
                lat=maxlat-1;
            }

            if ((val-lat)<0) {
                lon=lon*(-1)-1;
                lat=minlat;
            }     

            coordinate s = coordinate(lat,lon);   
            
            lat = s.lat;
            lon = s.lon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            coordinate c = coordinate(lat,lon);

            int row=lat+abs(minlat),col=lon+abs(minlon);
            row = clipped(row,0,maxlat-minlat-1);
            col = clipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }


        // Set/Get the value of the map cell at the given lat,lon using lat,lon arithmetics without screen OFFSET.
        //   (you can use this function when you can have lat,lon arithmetics that span the spheroid map)
        mapcell &peek(int lat, int lon)
        {
            int val = lat;
            val = ((int)val+0);
            lat = clipped(val,minlat,maxlat-1);

            lon = lon + 0;

            if (val>=maxlat) {
                lon=lon*(-1)-1;
                lat=maxlat-1;
            }

            if ((val-lat)<0) {
                lon=lon*(-1)-1;
                lat=minlat;
            }     

            coordinate s = coordinate(lat,lon);   
            
            lat = s.lat;
            lon = s.lon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            coordinate c = coordinate(lat,lon);

            int row=lat+abs(minlat),col=lon+abs(minlon);
            row = clipped(row,0,maxlat-minlat-1);
            col = clipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }

        mapcell &set(int lat, int lon)
        {
            int val = lat;
            val = ((int)val+0);
            lat = clipped(val,minlat,maxlat-1);

            lon = lon + 0;

            if (val>=maxlat) {
                lon=lon*(-1)-1;
                lat=maxlat-1;
            }

            if ((val-lat)<0) {
                lon=lon*(-1)-1;
                lat=minlat;
            }     

            coordinate s = coordinate(lat,lon);   
            
            lat = s.lat;
            lon = s.lon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            coordinate c = coordinate(lat,lon);

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


        // Convert offset lat,lon (zero,zero can be shifted) to screen fixed lat,lon (zero,zero is the center of the screen)
        coordinate to_screen(int lat, int lon)
        {
            lat = lat - offsetlat;
            lon = lon - offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        // Convert SCREEN lat,lon (zero,zero is the center of the screen) to REAL lat,lon
        coordinate to_real(int lat, int lon)
        {
            lat = lat + offsetlat;
            lon = lon + offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        coordinate adjust(int lat, int lon, int i, int j) 
        {
            int val = lat;
            val = ((int)val+i); // val is 24
            lat = clipped(val,minlat,maxlat-1); // lat is clipped to 23

            lon = lon + j; // lon is 35

            if (val>=maxlat) {
                lon=lon*(-1)-1; // -35
                lat=maxlat-1; // lat is clipped to 23
            }

            if ((val-lat)<0) {
                lon=lon*(-1)-1;
                lat=minlat;
            }     

            lon += abs(minlon);  // -35 + 36 = 1

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        
        }

        // Convert SCREEN lat,lon (zero,zero is the center of the screen) to REAL lat,lon
        coordinate to_real_without_offset(int lat, int lon)
        {
            lat = lat;
            lon = lon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        coordinate to_real_without_offset(coordinate c)
        {
            return to_real_without_offset(c.lat,c.lon);
        }

        coordinate displacement(int lat, int lon, int pitch, int roll)
        {
            int val = lat;
            val = ((int)val+pitch);
            lat = clipped(val,minlat,maxlat-1);

            lon = lon + roll;

            if (val>=maxlat) {
                lon=lon*(-1)-1;
                lat=maxlat-1;
            }

            if ((val-lat)<0) {
                lon=lon*(-1)-1;
                lat=minlat;
            }     

            return coordinate(lat,lon);       
        }

};

class MapOriginal
{
    private:
        std::vector<std::vector<mapcell>> map;


    public:
        int offsetlat, offsetlon;
        int minlat;
        int maxlat;
        int minlon;
        int maxlon;

        void init(int halfheight, int halfwidth)
        {
            minlat = -halfheight;
            maxlat = halfheight;
            minlon = -halfwidth;
            maxlon = halfwidth;

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

        // Get the map cell at the SCREEN coordinates lat, lon.  For instance (zero,zero) is what should be at the center of the screen.
        mapcell &operator()(int lat, int lon)
        {
            int row=lat+abs(minlat)+offsetlat,col=lon+abs(minlon)+offsetlon;
            row = rotclipped(row,0,maxlat-minlat-1);
            col = rotclipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }


        // Set/Get the value of the map cell at the given lat,lon using lat,lon arithmetics without screen OFFSET.
        //   (you can use this function when you can have lat,lon arithmetics that span the spheroid map)
        mapcell &peek(int lat, int lon)
        {
            int row=lat+abs(minlat),col=lon+abs(minlon);
            row = rotclipped(row,0,maxlat-minlat-1);
            col = rotclipped(col,0,maxlon-minlon-1);
            return map[row][col];
        }


        // Set/Get the value of the map cell at the given lat,lon using REAL lat,lon (of the map data structure)
        //    These are CLIPPED, so if you move away from the boundaries, you are affecting the last value.
        //    LAT,LON need always to be clipped.
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

        // Convert offset lat,lon (zero,zero can be shifted) to screen fixed lat,lon (zero,zero is the center of the screen)
        coordinate to_screen(int lat, int lon)
        {
            lat = lat - offsetlat;
            lon = lon - offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        // Convert SCREEN lat,lon (zero,zero is the center of the screen) to REAL lat,lon
        coordinate to_real(int lat, int lon)
        {
            lat = lat + offsetlat;
            lon = lon + offsetlon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        // Convert SCREEN lat,lon (zero,zero is the center of the screen) to REAL lat,lon
        coordinate to_real_without_offset(int lat, int lon)
        {
            lat = lat;
            lon = lon;

            lon += abs(minlon);

            lon = rotclipped(lon,0,maxlon-minlon-1);

            lon -= abs(minlon);

            return coordinate(lat,lon);
        }

        coordinate to_real_without_offset(coordinate c)
        {
            return to_real_without_offset(c.lat,c.lon);
        }

        coordinate spheroid_displacement(int lat, int lon, int pitch, int roll)
        {
            int val = lat;
            val = ((int)val+pitch);
            lat = clipped(val,minlat,maxlat-1);

            lon = lon + roll;

            if (val>=maxlat) {
                lon=lon*(-1);
                lat=maxlat-1;
            }

            if ((val-lat)<0) {
                lon=lon*(-1);
                lat=minlat;
            }     

            return coordinate(lat,lon);       
        }

};

typedef MapSpheroid Map;


#endif   // MAPMODEL_H