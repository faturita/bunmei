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
        this->bioma = 1;
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
        void init()
        {
            int no_of_cols = 80;
            int no_of_rows = 60;
            int initial_value = 0;

            map.resize(no_of_rows, std::vector<mapcell>(no_of_cols, initial_value));
        }

        mapcell &operator()(int lat, int lon)
        {
            int x=lat+30,y=lon+40;
            x = clipped(x,0,59);
            y = clipped(y,0,79);
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

    tiles[2] = "assets/assets/terrain/arctic.png";
    tiles[3] = "assets/assets/terrain/desert.png";
    tiles[4] = "assets/assets/terrain/forest.png";
    tiles[5] = "assets/assets/terrain/grassland.png";
    tiles[6] = "assets/assets/terrain/hills.png";
    tiles[7] = "assets/assets/terrain/jungle.png";
    tiles[8] = "assets/assets/terrain/mountains.png";
    tiles[9] = "assets/assets/terrain/plains.png";
    tiles[10] = "assets/assets/terrain/river.png";
    tiles[11] = "assets/assets/terrain/swamp.png";
    tiles[12] = "assets/assets/terrain/tundra.png";

    map.init();

    for(int lat=-30;lat<30;lat++)
        for (int lon=-40;lon<40;lon++)
        {
            map(lat,lon) = mapcell(0);
        }

    int r=getRandomInteger(2,15);

    for(int i=0;i<r;i++)
    {
        int lat = getRandomInteger(-30,29);
        int lon = getRandomInteger(-40,39);

        while (getRandomInteger(0,100)>2)
        {
            map(lat,lon) = mapcell(1);
            int dir=getRandomInteger(0,3);
            if (dir==0) lat+=1;
            if (dir==1) lat-=1;
            if (dir==2) lon+=1;
            if (dir==3) lon-=1;

        }
    }

    int energy = 100000;
    for(int rep=0;rep<5000;rep++)
    {
        int lat = getRandomInteger(-30,29);
        int lon = getRandomInteger(-40,39);

        int north,south,east,west;
        north = map(lat+1,lon).code;
        south = map(lat-1,lon).code;
        east  = map(lat,lon+1).code;
        west  = map(lat,lon-1).code;

        if (energy>0) if ((south+north+east+west)>=1)
        {
            map(lat,lon) = mapcell(1);
            energy--;
        }
    }

    for(int lat=-30;lat<30;lat++)
        for (int lon=-40;lon<40;lon++)
        {
            int north,south,east,west;
            north = map(lat+1,lon).code;
            south = map(lat-1,lon).code;
            east  = map(lat,lon+1).code;
            west  = map(lat,lon-1).code;

            if (energy>0) if ((south+north+east+west)>=3)
            {
                map(lat,lon) = mapcell(1);
            }
        }


    for(int i=0;i<100;i++)
    {
        int lat = getRandomInteger(-30,29);
        int lon = getRandomInteger(-40,39);

        int bioma = getRandomInteger(2,12);

        while (getRandomInteger(0,100)>2)
        {
            if (map(lat,lon).code==1)
            {
                map(lat,lon).bioma = bioma;
                int dir=getRandomInteger(0,3);
                if (dir==0) lat+=1;
                if (dir==1) lat-=1;
                if (dir==2) lon+=1;
                if (dir==3) lon-=1;
            }

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
    // @FIXME: Parametrize all the resolution values.
    int xsize = 1200/mapzoom;
    int ysize = 800/mapzoom;

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

        for(int lat=-30;lat<30;lat++)
        {
            for(int lon=-40;lon<40;lon++)
            {
                if (map(lat,lon).visible) place(lon,lat,16,tiles[map(lat,lon).code].c_str());
            }
        }

        for(int lat=-30;lat<30;lat++)
        {
            for(int lon=-40;lon<40;lon++)
            {
                if (map(lat,lon).visible &&map(lat,lon).code==1) place(lon,lat,16,tiles[map(lat,lon).bioma].c_str());
            }
        }

        for(int lat=-30;lat<30;lat++)
            for(int lon=-40;lon<40;lon++)
            {
                int land = map(lat,lon).code;
                mapcell next = map.south(lat,lon);
                coordinate c = map.isouth(lat,lon);

                if (map(lat,lon).visible) if (land == 1 && next.code == 0)
                {
                    placeMark(600+16*c.lon-4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_s2.png");
                    placeMark(600+16*c.lon+4, 0+16*c.lat+8,    8,"assets/assets/terrain/coast_s2.png");
                }


                next = map.north(lat,lon);
                c = map.inorth(lat,lon);

                if (map(lat,lon).visible) if (land == 1 && next.code == 0)
                {
                    placeMark(600+16*c.lon-4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_n2.png");
                    placeMark(600+16*c.lon+4, 0+16*c.lat-8,    8,"assets/assets/terrain/coast_n2.png");
                }

                next = map.east(lat,lon);
                c = map.ieast(lat,lon);

                if (map(lat,lon).visible) if (land == 1 && next.code == 0)
                {
                    placeMark(600+16*c.lon-8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_e2.png");
                    placeMark(600+16*c.lon-8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_e2.png");
                }

                next = map.west(lat,lon);
                c = map.iwest(lat,lon);

                if (map(lat,lon).visible) if (land == 1 && next.code == 0)
                {
                    placeMark(600+16*c.lon+8, 0+16*c.lat-4,    8,"assets/assets/terrain/coast_w2.png");
                    placeMark(600+16*c.lon+8, 0+16*c.lat+4,    8,"assets/assets/terrain/coast_w2.png");
                }

            }


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
            for(int lat=-30;lat<30;lat++)
                for(int lon=-40;lon<40;lon++)
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

        for(int lat=-30;lat<30;lat++)
            for(int lon=-40;lon<40;lon++)
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
            placeThisUnit(controller.registers.roll,controller.registers.pitch,16,"assets/assets/units/settlers.png");

        //placeThisUnit(controller.registers.roll,controller.registers.pitch,16,"assets/assets/units/trireme.png");


        glDisable(GL_BLEND);

    } glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

}




