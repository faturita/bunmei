#include "openglutils.h"
#include "math/yamathutil.h"
#include "lodepng.h"
#include "font/FontsBitmap.h"
#include "font/DrawFonts.h"
#include "map.h"
#include "City.h"
#include "ui.h"

extern float cx;
extern float cy;

extern Map map;

void drawBoundingBox(int clo,int cla, int startleft, int starttop, int endright, int endbottom)
{

    place((clo + (startleft))*16,(cla + (starttop))*16,16,16,"assets/assets/general/topleft.png");
    for(int i=startleft+1;i<endright;i++)
    {
        place((clo + (i))*16,(cla + (starttop))*16,16,16,"assets/assets/general/top.png");
        place((clo + (i))*16,(cla + (endbottom))*16,16,16,"assets/assets/general/bottom.png");
    }
    place((clo + (endright))*16 ,(cla + (starttop))*16,16,16,"assets/assets/general/topright.png");
    place((clo + (startleft))*16 ,(cla + (endbottom))*16,16,16,"assets/assets/general/bottomleft.png");
    for(int i=starttop+1;i<endbottom;i++)
    {
        place((clo + (startleft))*16 ,(cla + (i))*16,16,16,"assets/assets/general/left.png");
        place((clo + (endright))*16 ,(cla + (i))*16,16,16,"assets/assets/general/right.png");
    }
    place((clo + (endright))*16 ,(cla + (endbottom))*16,16,16,"assets/assets/general/bottomright.png");

}



void drawCityScreen(int cla, int clo, City *city)
{
    for(int lats=-10;lats<10;lats++)
        for (int lons=-10;lons<10;lons++)
        {
            if ( (lats<-3 || lats>3) || (lons<-3 || lons>3) )
            {
                int la= cla + lats;
                int lo = clo + lons;


                placeTile(lo,la,"assets/assets/general/citytexture.png");

            }
        }

    drawBoundingBox(clo,cla,-3,-3,3,3);

    placeWord(clo + (-10),cla + (-10),4,8,city->name);


    place((clo + (-10))*16,(cla + (-9))*16,8,16,"assets/assets/city/people_content_m.png");
    place((clo + (-10))*16+4,(cla + (-9))*16,8,16,"assets/assets/city/people_content_f.png");

    placeWord(clo + (-10),cla + (-8),4,8,"City Resources");
    drawBoundingBox(clo,cla,-10,-8,-4,-4);

    placeWord(clo + (-10),cla + (-3),4,8,"Food Storage");
    drawBoundingBox(clo,cla,-10,-3,-4,9);

    for(int i=0;i<city->resources[0];i++)
        place((clo+(-10))*16-4+7*(i%10)  ,(cla+(-2))*16-4+7*(i/10)  ,7,7,"assets/assets/city/food.png");


    placeWord(clo + (4),cla + (-10),4,8,"PALACE");
    drawBoundingBox(clo,cla,4,-10,9,-1);

    placeWord(clo + (4),cla + (4),4,8,"Change");  // Row, Column
    drawBoundingBox(clo,cla,4,4,9,9);


    for(int lats=-3;lats<=3;lats++)
        for(int lons=-3;lons<=3;lons++)
        {
            int la= cla + lats;
            int lo = clo + lons;

            place((lo)*16-4  ,(la)*16-4  ,7,7,"assets/assets/city/food.png");
            place((lo)*16-4+7,(la)*16-4  ,7,7,"assets/assets/city/food.png");
            place((lo)*16-4  ,(la)*16-4+7,7,7,"assets/assets/city/production.png");
            place((lo)*16-4+7,(la)*16-4+7,7,7,"assets/assets/city/trade.png");
        }

}