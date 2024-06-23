#include <vector>

#include <iostream>
#include <unordered_map>
#include <vector>
#include "openglutils.h"
#include "lodepng.h"
#include "usercontrols.h"
#include "math/yamathutil.h"
#include "map.h"


// 1200 x 800
// 1920 x 1080
int width = 1200;
int height = 800;
int mapzoom=1;

float cx,cy;

std::unordered_map<std::string, GLuint> maptextures;
std::unordered_map<int, std::string> tiles;
extern Controller controller;

struct mapcell
{
    mapcell(int code)
    {
        this->code = code;
        this->visible = true;
        this->bioma = 0;// By default, nothing
    }

    int code;
    bool visible;
    int bioma;
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

};

Map map;


GLuint preloadTexture(const char* modelName)
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

    return _texture;
}



void initMap()
{
    tiles[0] = "assets/assets/terrain/ocean.png";
    tiles[1] = "assets/assets/terrain/land.png";

    tiles[2] = "assets/assets/terrain/river_mouth_w.png";
    tiles[3] = "assets/assets/terrain/river_mouth_s.png";
    tiles[4] = "assets/assets/terrain/river_mouth_e.png";
    tiles[5] = "assets/assets/terrain/river_mouth_n.png";

    tiles[0x20] = "assets/assets/terrain/arctic.png";
    tiles[0x30] = "assets/assets/terrain/desert.png";
    tiles[0x40] = "assets/assets/terrain/forest.png";
    tiles[0x50] = "assets/assets/terrain/grassland.png";
    tiles[0x60] = "assets/assets/terrain/hills.png";
    tiles[0x70] = "assets/assets/terrain/jungle.png";
    tiles[0x80] = "assets/assets/terrain/mountains.png";
    tiles[0x90] = "assets/assets/terrain/plains.png";
    tiles[0xa0] = "assets/assets/terrain/river.png";
    tiles[0xb0] = "assets/assets/terrain/swamp.png";
    tiles[0xc0] = "assets/assets/terrain/tundra.png";

    tiles[0x20] = "assets/assets/terrain/arctic.png";
    tiles[0x21] = "assets/assets/terrain/arctic_w.png";
    tiles[0x22] = "assets/assets/terrain/arctic_s.png";
    tiles[0x23] = "assets/assets/terrain/arctic_sw.png";
    tiles[0x24] = "assets/assets/terrain/arctic_e.png";
    tiles[0x25] = "assets/assets/terrain/arctic_ew.png";
    tiles[0x26] = "assets/assets/terrain/arctic_es.png";
    tiles[0x27] = "assets/assets/terrain/arctic_esw.png";
    tiles[0x28] = "assets/assets/terrain/arctic_n.png";
    tiles[0x29] = "assets/assets/terrain/arctic_nw.png";
    tiles[0x2a] = "assets/assets/terrain/arctic_ns.png";
    tiles[0x2b] = "assets/assets/terrain/arctic_nsw.png";
    tiles[0x2c] = "assets/assets/terrain/arctic_ne.png";
    tiles[0x2d] = "assets/assets/terrain/arctic_new.png";
    tiles[0x2e] = "assets/assets/terrain/arctic_nes.png";
    tiles[0x2f] = "assets/assets/terrain/arctic_nesw.png";

    tiles[0x30] = "assets/assets/terrain/desert.png";
    tiles[0x31] = "assets/assets/terrain/desert_w.png";
    tiles[0x32] = "assets/assets/terrain/desert_s.png";
    tiles[0x33] = "assets/assets/terrain/desert_sw.png";
    tiles[0x34] = "assets/assets/terrain/desert_e.png";
    tiles[0x35] = "assets/assets/terrain/desert_ew.png";
    tiles[0x36] = "assets/assets/terrain/desert_es.png";
    tiles[0x37] = "assets/assets/terrain/desert_esw.png";
    tiles[0x38] = "assets/assets/terrain/desert_n.png";
    tiles[0x39] = "assets/assets/terrain/desert_nw.png";
    tiles[0x3a] = "assets/assets/terrain/desert_ns.png";
    tiles[0x3b] = "assets/assets/terrain/desert_nsw.png";
    tiles[0x3c] = "assets/assets/terrain/desert_ne.png";
    tiles[0x3d] = "assets/assets/terrain/desert_new.png";
    tiles[0x3e] = "assets/assets/terrain/desert_nes.png";
    tiles[0x3f] = "assets/assets/terrain/desert_nesw.png";

    tiles[0x40] = "assets/assets/terrain/forest.png";
    tiles[0x41] = "assets/assets/terrain/forest_w.png";
    tiles[0x42] = "assets/assets/terrain/forest_s.png";
    tiles[0x43] = "assets/assets/terrain/forest_sw.png";
    tiles[0x44] = "assets/assets/terrain/forest_e.png";
    tiles[0x45] = "assets/assets/terrain/forest_ew.png";
    tiles[0x46] = "assets/assets/terrain/forest_es.png";
    tiles[0x47] = "assets/assets/terrain/forest_esw.png";
    tiles[0x48] = "assets/assets/terrain/forest_n.png";
    tiles[0x49] = "assets/assets/terrain/forest_nw.png";
    tiles[0x4a] = "assets/assets/terrain/forest_ns.png";
    tiles[0x4b] = "assets/assets/terrain/forest_nsw.png";
    tiles[0x4c] = "assets/assets/terrain/forest_ne.png";
    tiles[0x4d] = "assets/assets/terrain/forest_new.png";
    tiles[0x4e] = "assets/assets/terrain/forest_nes.png";
    tiles[0x4f] = "assets/assets/terrain/forest_nesw.png";

    tiles[0x50] = "assets/assets/terrain/grassland.png";
    tiles[0x51] = "assets/assets/terrain/grassland_w.png";
    tiles[0x52] = "assets/assets/terrain/grassland_s.png";
    tiles[0x53] = "assets/assets/terrain/grassland_sw.png";
    tiles[0x54] = "assets/assets/terrain/grassland_e.png";
    tiles[0x55] = "assets/assets/terrain/grassland_ew.png";
    tiles[0x56] = "assets/assets/terrain/grassland_es.png";
    tiles[0x57] = "assets/assets/terrain/grassland_esw.png";
    tiles[0x58] = "assets/assets/terrain/grassland_n.png";
    tiles[0x59] = "assets/assets/terrain/grassland_nw.png";
    tiles[0x5a] = "assets/assets/terrain/grassland_ns.png";
    tiles[0x5b] = "assets/assets/terrain/grassland_nsw.png";
    tiles[0x5c] = "assets/assets/terrain/grassland_ne.png";
    tiles[0x5d] = "assets/assets/terrain/grassland_new.png";
    tiles[0x5e] = "assets/assets/terrain/grassland_nes.png";
    tiles[0x5f] = "assets/assets/terrain/grassland_nesw.png";
    
    tiles[0x60] = "assets/assets/terrain/hills.png";
    tiles[0x61] = "assets/assets/terrain/hills_w.png";
    tiles[0x62] = "assets/assets/terrain/hills_s.png";
    tiles[0x63] = "assets/assets/terrain/hills_sw.png";
    tiles[0x64] = "assets/assets/terrain/hills_e.png";
    tiles[0x65] = "assets/assets/terrain/hills_ew.png";
    tiles[0x66] = "assets/assets/terrain/hills_es.png";
    tiles[0x67] = "assets/assets/terrain/hills_esw.png";
    tiles[0x68] = "assets/assets/terrain/hills_n.png";
    tiles[0x69] = "assets/assets/terrain/hills_nw.png";
    tiles[0x6a] = "assets/assets/terrain/hills_ns.png";
    tiles[0x6b] = "assets/assets/terrain/hills_nsw.png";
    tiles[0x6c] = "assets/assets/terrain/hills_ne.png";
    tiles[0x6d] = "assets/assets/terrain/hills_new.png";
    tiles[0x6e] = "assets/assets/terrain/hills_nes.png";
    tiles[0x6f] = "assets/assets/terrain/hills_nesw.png";

    tiles[0x70] = "assets/assets/terrain/jungle.png";
    tiles[0x71] = "assets/assets/terrain/jungle_w.png";
    tiles[0x72] = "assets/assets/terrain/jungle_s.png";
    tiles[0x73] = "assets/assets/terrain/jungle_sw.png";
    tiles[0x74] = "assets/assets/terrain/jungle_e.png";
    tiles[0x75] = "assets/assets/terrain/jungle_ew.png";
    tiles[0x76] = "assets/assets/terrain/jungle_es.png";
    tiles[0x77] = "assets/assets/terrain/jungle_esw.png";
    tiles[0x78] = "assets/assets/terrain/jungle_n.png";
    tiles[0x79] = "assets/assets/terrain/jungle_nw.png";
    tiles[0x7a] = "assets/assets/terrain/jungle_ns.png";
    tiles[0x7b] = "assets/assets/terrain/jungle_nsw.png";
    tiles[0x7c] = "assets/assets/terrain/jungle_ne.png";
    tiles[0x7d] = "assets/assets/terrain/jungle_new.png";
    tiles[0x7e] = "assets/assets/terrain/jungle_nes.png";
    tiles[0x7f] = "assets/assets/terrain/jungle_nesw.png";

    tiles[0x80] = "assets/assets/terrain/mountains.png";
    tiles[0x81] = "assets/assets/terrain/mountains_w.png";
    tiles[0x82] = "assets/assets/terrain/mountains_s.png";
    tiles[0x83] = "assets/assets/terrain/mountains_sw.png";
    tiles[0x84] = "assets/assets/terrain/mountains_e.png";
    tiles[0x85] = "assets/assets/terrain/mountains_ew.png";
    tiles[0x86] = "assets/assets/terrain/mountains_es.png";
    tiles[0x87] = "assets/assets/terrain/mountains_esw.png";
    tiles[0x88] = "assets/assets/terrain/mountains_n.png";
    tiles[0x89] = "assets/assets/terrain/mountains_nw.png";
    tiles[0x8a] = "assets/assets/terrain/mountains_ns.png";
    tiles[0x8b] = "assets/assets/terrain/mountains_nsw.png";
    tiles[0x8c] = "assets/assets/terrain/mountains_ne.png";
    tiles[0x8d] = "assets/assets/terrain/mountains_new.png";
    tiles[0x8e] = "assets/assets/terrain/mountains_nes.png";
    tiles[0x8f] = "assets/assets/terrain/mountains_nesw.png";

    tiles[0x90] = "assets/assets/terrain/plains.png";
    tiles[0x91] = "assets/assets/terrain/plains_w.png";
    tiles[0x92] = "assets/assets/terrain/plains_s.png";
    tiles[0x93] = "assets/assets/terrain/plains_sw.png";
    tiles[0x94] = "assets/assets/terrain/plains_e.png";
    tiles[0x95] = "assets/assets/terrain/plains_ew.png";
    tiles[0x96] = "assets/assets/terrain/plains_es.png";
    tiles[0x97] = "assets/assets/terrain/plains_esw.png";
    tiles[0x98] = "assets/assets/terrain/plains_n.png";
    tiles[0x99] = "assets/assets/terrain/plains_nw.png";
    tiles[0x9a] = "assets/assets/terrain/plains_ns.png";
    tiles[0x9b] = "assets/assets/terrain/plains_nsw.png";
    tiles[0x9c] = "assets/assets/terrain/plains_ne.png";
    tiles[0x9d] = "assets/assets/terrain/plains_new.png";
    tiles[0x9e] = "assets/assets/terrain/plains_nes.png";
    tiles[0x9f] = "assets/assets/terrain/plains_nesw.png";

    tiles[0xa0] = "assets/assets/terrain/river.png";
    tiles[0xa1] = "assets/assets/terrain/river_w.png";
    tiles[0xa2] = "assets/assets/terrain/river_s.png";
    tiles[0xa3] = "assets/assets/terrain/river_sw.png";
    tiles[0xa4] = "assets/assets/terrain/river_e.png";
    tiles[0xa5] = "assets/assets/terrain/river_ew.png";
    tiles[0xa6] = "assets/assets/terrain/river_es.png";
    tiles[0xa7] = "assets/assets/terrain/river_esw.png";
    tiles[0xa8] = "assets/assets/terrain/river_n.png";
    tiles[0xa9] = "assets/assets/terrain/river_nw.png";
    tiles[0xaa] = "assets/assets/terrain/river_ns.png";
    tiles[0xab] = "assets/assets/terrain/river_nsw.png";
    tiles[0xac] = "assets/assets/terrain/river_ne.png";
    tiles[0xad] = "assets/assets/terrain/river_new.png";
    tiles[0xae] = "assets/assets/terrain/river_nes.png";
    tiles[0xaf] = "assets/assets/terrain/river_nesw.png";

    tiles[0xb0] = "assets/assets/terrain/swamp.png";
    tiles[0xb1] = "assets/assets/terrain/swamp_w.png";
    tiles[0xb2] = "assets/assets/terrain/swamp_s.png";
    tiles[0xb3] = "assets/assets/terrain/swamp_sw.png";
    tiles[0xb4] = "assets/assets/terrain/swamp_e.png";
    tiles[0xb5] = "assets/assets/terrain/swamp_ew.png";
    tiles[0xb6] = "assets/assets/terrain/swamp_es.png";
    tiles[0xb7] = "assets/assets/terrain/swamp_esw.png";
    tiles[0xb8] = "assets/assets/terrain/swamp_n.png";
    tiles[0xb9] = "assets/assets/terrain/swamp_nw.png";
    tiles[0xba] = "assets/assets/terrain/swamp_ns.png";
    tiles[0xbb] = "assets/assets/terrain/swamp_nsw.png";
    tiles[0xbc] = "assets/assets/terrain/swamp_ne.png";
    tiles[0xbd] = "assets/assets/terrain/swamp_new.png";
    tiles[0xbe] = "assets/assets/terrain/swamp_nes.png";
    tiles[0xbf] = "assets/assets/terrain/swamp_nesw.png";

    tiles[0xc0] = "assets/assets/terrain/tundra.png";
    tiles[0xc1] = "assets/assets/terrain/tundra_w.png";
    tiles[0xc2] = "assets/assets/terrain/tundra_s.png";
    tiles[0xc3] = "assets/assets/terrain/tundra_sw.png";
    tiles[0xc4] = "assets/assets/terrain/tundra_e.png"; 
    tiles[0xc5] = "assets/assets/terrain/tundra_ew.png";
    tiles[0xc6] = "assets/assets/terrain/tundra_es.png";
    tiles[0xc7] = "assets/assets/terrain/tundra_esw.png";
    tiles[0xc8] = "assets/assets/terrain/tundra_n.png";
    tiles[0xc9] = "assets/assets/terrain/tundra_nw.png";
    tiles[0xca] = "assets/assets/terrain/tundra_ns.png";
    tiles[0xcb] = "assets/assets/terrain/tundra_nsw.png";
    tiles[0xcc] = "assets/assets/terrain/tundra_ne.png";
    tiles[0xcd] = "assets/assets/terrain/tundra_new.png";
    tiles[0xce] = "assets/assets/terrain/tundra_nes.png";
    tiles[0xcf] = "assets/assets/terrain/tundra_nesw.png";

    map.init();

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(0);
        }

    // for(int lat=-3;lat<=3;lat++)
    //     for (int lon=-3;lon<=3;lon++)
    //     {
    //         map.set(lat,lon) = mapcell(1);
    //     }

    // map.set(0,-4).bioma = 0xa0;
    // map.set(0,-3).bioma = 0xa0;
    // map.set(0,-2).bioma = 0xa0;
    // map.set(0,-1).bioma = 0xa0;

    // map.set(0,4).bioma = 0xa0;
    // map.set(0,3).bioma = 0xa0;
    // map.set(0,2).bioma = 0xa0;
    // map.set(0,1).bioma = 0xa0;

    // map.set(-4,0).bioma = 0xa0;
    // map.set(-3,0).bioma = 0xa0;
    // map.set(-2,0).bioma = 0xa0;
    // map.set(-1,0).bioma = 0xa0;


    // map.set(4,0).bioma = 0xa0;
    // map.set(3,0).bioma = 0xa0;
    // map.set(2,0).bioma = 0xa0;
    // map.set(1,0).bioma = 0xa0;

    // map.set(0,0).bioma = 0xa0;


    // Pick a random number of land masses seeds.
    int r=getRandomInteger(2,15);

    for(int i=0;i<r;i++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        while (getRandomInteger(0,100)>2)
        {
            map.set(lat,lon) = mapcell(1);
            map.set(lat,lon).bioma = 1;
            int dir=getRandomInteger(0,3);
            if (dir==0) lat+=1;
            if (dir==1) lat-=1;
            if (dir==2) lon+=1;
            if (dir==3) lon-=1;

        }
    }

    // Fill in randomly the land masses with land.
    int energy = 100000;
    for(int rep=0;rep<5000;rep++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        int north,south,east,west;
        north = map(lat+1,lon).code;
        south = map(lat-1,lon).code;
        east  = map(lat,lon+1).code;
        west  = map(lat,lon-1).code;

        if (energy>0) if ((south+north+east+west)>=1)
        {
            map.set(lat,lon) = mapcell(1);
            energy--;
        }
    }

    // Now go through all the spots and fill in with more land to fill the gaps.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            int north,south,east,west;
            north = map(lat+1,lon).code;
            south = map(lat-1,lon).code;
            east  = map(lat,lon+1).code;
            west  = map(lat,lon-1).code;

            if (energy>0) if ((south+north+east+west)>=3)
            {
                map.set(lat,lon) = mapcell(1);
            }
        }


    // Pick the biomas.
    for(int i=0;i<100;i++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        int biomalist[] = {0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xb0,0xc0};

        int bioma = biomalist[getRandomInteger(0,9)];

        while (getRandomInteger(0,100)>2)
        {
            if (map(lat,lon).code==1)
            {
                map.set(lat,lon).bioma = bioma;
                int dir=getRandomInteger(0,3);
                if (dir==0) lat+=1;
                if (dir==1) lat-=1;
                if (dir==2) lon+=1;
                if (dir==3) lon-=1;
            }

        }
    }


    // Put the rivers
    for(int i=0;i<100;i++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        int bioma = 0xa0;

        int dir=0;

        while (map(lat,lon).code==1)
        {
            map.set(lat,lon).bioma = bioma;

            if (map.north(lat,lon).code==0)
            {
                dir=0;
            } else if (map.south(lat,lon).code==0)
            {
                dir=1;
            } else if (map.east(lat,lon).code==0)
            {
                dir=2;
            } else if (map.west(lat,lon).code==0)
            {
                dir=3;
            } else {
                dir=getRandomInteger(0,3);
            }
            if (dir==0) lat+=1;
            if (dir==1) lat-=1;
            if (dir==2) lon+=1;
            if (dir==3) lon-=1;

            if (map(lat,lon).code==0) 
            {
                map.set(lat,lon).bioma = 0xa0;
            }
        }
    }


    //map.set(-8,-4).code = 0;
    //map.set(-8,-4).bioma = 0xa0;


    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (map(lat,lon).code==1 && map(lat,lon).bioma>5)       // Skip all the first biomas which are river mouths
            {
                int bioma = map(lat,lon).bioma;

                int biom = bioma & 0xf0;

                int b1 = (map.west(lat,lon).bioma & 0xf0 ^ biom)>0;b1=b1?0:1;
                int b2 = (map.south(lat,lon).bioma & 0xf0 ^ biom)>0;b2=b2?0:1;
                int b3 = (map.east(lat,lon).bioma & 0xf0 ^ biom)>0;b3=b3?0:1;
                int b4 = (map.north(lat,lon).bioma & 0xf0 ^ biom)>0;b4=b4?0:1;

                //printf(" %x %x %x %x: %x\n",b4,b3,b2,b1, b4<<3 | b3<<2 | b2<<1 | b1);

                bioma += (b4<<3 | b3<<2 | b2<<1 | b1);

                //printf("Lat %d Lon %d Bioma: %x\n",lat,lon,bioma);

                map.set(lat,lon).bioma = bioma;

            }
        }

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (map(lat,lon).code==0 && map(lat,lon).bioma==0xa0)
            {
                int biom = 0xa0;
                int b1 = (map.west(lat,lon).bioma & 0xf0 ^ biom)>0;b1=b1?0:1;
                int b2 = (map.south(lat,lon).bioma & 0xf0 ^ biom)>0;b2=b2?0:1;
                int b3 = (map.east(lat,lon).bioma & 0xf0 ^ biom)>0;b3=b3?0:1;
                int b4 = (map.north(lat,lon).bioma & 0xf0 ^ biom)>0;b4=b4?0:1;

                printf(" %x %x %x %x: %x\n",b4,b3,b2,b1, b4<<3 | b3<<2 | b2<<1 | b1);

                int val = (b4<<3 | b3<<2 | b2<<1 | b1);

                if (b1) map.set(lat,lon).bioma = 2;
                if (b2) map.set(lat,lon).bioma = 3;
                if (b3) map.set(lat,lon).bioma = 4;
                if (b4) map.set(lat,lon).bioma = 5;

            }
        }

}


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
    //1728,1117

    float fccx = ccx/1728.0*1200.0;
    float fccy = ccy/1117.0*800.0;

    ccx = fccx;
    ccy = fccy;

    // @FIXME: Parametrize all the resolution values.  This depends on the screen resolution, and the amount of clickable space
    int xsize = 1200/mapzoom;   //1200
    int ysize = 800/mapzoom;    //800

    // Screen width and height come from OpenGL.
    cx = (int)(ccx*(xsize)/(float)width)+cx-xsize/2; // 1440
    cy = (int)(ccy*(ysize)/(float)height)+cy-ysize/2;  // 900
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

    placeMark(x, y, size, size, _texture);
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

    placeMark(x, y, size, size, _texture);
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
    GLuint _texture = preloadTexture(modelName);
    placeMark(x,y,size,size,_texture);
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

        // Water or Land
        for(int lat=map.minlat;lat<map.maxlat;lat++)
        {
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).visible) place(lon,lat,16,tiles[map(lat,lon).code].c_str());
            }
        }

        // Biomas, including biomas on ocean (like river mouths)
        for(int lat=map.minlat;lat<map.maxlat;lat++)
        {
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).visible && map(lat,lon).bioma!=0) place(lon,lat,16,tiles[map(lat,lon).bioma].c_str());
            }
        }

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
                        placeMark(600+16*c.lon-4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_s2.png");
                        placeMark(600+16*c.lon+4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_s2.png");
                    }
                    else
                    {
                        placeMark(600+16*c.lon-4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_sm1.png");
                        placeMark(600+16*c.lon+4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_sm2.png");
                    }
                }


                next = map.north(lat,lon);
                c = map.inorth(lat,lon);

                if (map(lat,lon).visible && land == 1 && next.code == 0)
                {
                    if ((map(lat,lon).bioma & 0xf8) != 0xa8)
                    {
                        placeMark(600+16*c.lon-4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_n2.png");
                        placeMark(600+16*c.lon+4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_n2.png");
                    }
                    else
                    {
                        placeMark(600+16*c.lon-4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_nm2.png");
                        placeMark(600+16*c.lon+4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_nm1.png");
                    }
                }

                next = map.east(lat,lon);
                c = map.ieast(lat,lon);

                if (map(lat,lon).visible && land == 1 && next.code == 0)
                {
                    if ((map(lat,lon).bioma & 0xf4) != 0xa4)
                    {
                        placeMark(600+16*c.lon-8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_e5.png");
                        placeMark(600+16*c.lon-8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_e5.png");
                    }
                    else
                    {
                        placeMark(600+16*c.lon-8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_em1.png");
                        placeMark(600+16*c.lon-8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_em2.png");
                    }
                }

                next = map.west(lat,lon);
                c = map.iwest(lat,lon);

                if (map(lat,lon).visible && land == 1 && next.code == 0)
                {
                    if ((map(lat,lon).bioma & 0xf1) != 0xa1) 
                    {
                        placeMark(600+16*c.lon+8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_w2.png");
                        placeMark(600+16*c.lon+8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_w2.png");
                    } 
                    else 
                    {
                        placeMark(600+16*c.lon+8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_wm2.png");
                        placeMark(600+16*c.lon+8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_wm1.png");
                    }
                }
            }

        /**for(int lat=map.minlat;lat<map.maxlat;lat++)
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                int land = map(lat,lon).code;
                mapcell next = map.south(lat,lon);
                coordinate c = map.isouth(lat,lon);

                if (map(lat,lon).visible && (map(lat,lon).bioma & 0xf2) == 0xa2) if (land == 1 && next.code == 0)
                {
                    place(c.lon,c.lat,16,tiles[5].c_str());
                }


                next = map.north(lat,lon);
                c = map.inorth(lat,lon);

                if (map(lat,lon).visible&& (map(lat,lon).bioma & 0xf8) == 0xa8) if (land == 1 && next.code == 0)
                {
                    place(c.lon,c.lat,16,tiles[3].c_str());
                }

                next = map.east(lat,lon);
                c = map.ieast(lat,lon);

                if (map(lat,lon).visible&& (map(lat,lon).bioma & 0xf4) == 0xa4) if (land == 1 && next.code == 0)
                {
                    place(c.lon,c.lat,16,tiles[2].c_str());
                }

                next = map.west(lat,lon);
                c = map.iwest(lat,lon);

                if (map(lat,lon).visible&& (map(lat,lon).bioma & 0xf1) == 0xa1) if (land == 1 && next.code == 0)
                {
                    place(c.lon,c.lat,16,tiles[4].c_str());
                }
            }**/


        /**
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

        static int count=0;
        if (count++ % 100 < 50)
            placeThisUnit(controller.registers.roll,controller.registers.pitch,16,"assets/assets/units/settlers.png");

        **/

        static int count=0;

        std::vector<coordinate> list;
        if (count==0)
        {
            for(int lat=map.minlat;lat<map.maxlat;lat++)
                for(int lon=map.minlon;lon<map.maxlon;lon++)
                {
                    if (map(lat,lon).code==1)
                    {
                        list.push_back(coordinate(lat,lon));
                    }
                }

            coordinate c(0,0);
            if (list.size()>0)
            {
                int r = getRandomInteger(0,list.size());
                c = list[r];
            }
            controller.registers.pitch = c.lat;
            controller.registers.roll = c.lon;

        }

        coordinate c = coordinate(controller.registers.pitch,controller.registers.roll);
        for(int llat=-1;llat<=1;llat++)
            for (int llon=-1;llon<=1;llon++)
            {
                int lllat=c.lat+llat,lllon=c.lon+llon;
                map(lllat,lllon).visible = true;
            }

        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).visible)
                {
                    if (!(map.south(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_s.png");
                    if (!(map.north(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_n.png");
                    if (!(map.west(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_w.png");
                    if (!(map.east(lat,lon).visible)) place(lon,lat,16,"assets/assets/map/fog_e.png");

                }
            }


        if (count++ % 100 < 50)
        {
            int lat = ((int)controller.registers.pitch) + abs(map.minlat) - map.centerx;
            lat = lat % (map.maxlat-map.minlat);
            if (lat<0) lat = (map.maxlat-map.minlat) + lat;
            lat = lat - abs(map.minlat);
            int lon = ((int)controller.registers.roll) + abs(map.minlon) - map.centery;
            lon = lon % (map.maxlon-map.minlon);
            if (lon<0) lon = (map.maxlon-map.minlon) + lon;
            lon = lon - abs(map.minlon);

            placeThisUnit(lon,lat,16,"assets/assets/units/settlers.png");
        }

        map.setCenter(0,controller.registers.yaw);

        glDisable(GL_BLEND);

    } glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

}




