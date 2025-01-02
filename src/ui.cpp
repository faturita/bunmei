#include "openglutils.h"
#include "math/yamathutil.h"
#include "lodepng.h"
#include "font/FontsBitmap.h"
#include "font/DrawFonts.h"
#include "map.h"
#include "resources.h"
#include "City.h"
#include "ui.h"

extern float cx;
extern float cy;

extern Map map;
extern std::vector<Resource*> resources;

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

// @FIXME: This is super ugly, but it works for now.
bool changeIsActive = false;
int selection = -1;
int baselat = 11;

coordinate clickedTile(0,0);
bool tileWorkingIsActive = false;

void clickOnCityScreen(int lat, int lon, int lat2, int lon2)
{
    printf("Click on City Screen %d,%d - %d, %d\n",lat,lon, lat2, lon2);
    if ((lat==4 && lon==5) || (lat==4 && lon==4))
    {
        printf("Change\n"); // Row, Column
        changeIsActive = true;
        if (lat2==8) baselat = 10;
        else baselat = 11;
    }

    if (changeIsActive)
    {
        selection = lat2 - baselat;
        printf("Selection %d\n",selection);
    }

    if ((lat>-3 || lat<3) && (lon>-3 || lon<3))
    {
        printf("Tile Working....\n");
        tileWorkingIsActive = true;
        clickedTile = coordinate(lat,lon);
    }


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

    drawBoundingBox(clo,cla,-10,-10,9,9);
    drawBoundingBox(clo,cla,-3,-3,3,3);

    placeWord(clo + (-10),cla + (-10),4,8,city->name);

    for(int i=0;i<city->pop;i++)
    {
        if (i%2==0)
            place((clo + (-10))*16+4*i,(cla + (-9))*16,8,16,"assets/assets/city/people_content_m.png");
        else
            place((clo + (-10))*16+4*i,(cla + (-9))*16,8,16,"assets/assets/city/people_content_f.png");
    }
    
    
    {
        /// Show the production rate for all the resources from the tiles of the city (@FIXME add the resources produced by buildings and units)
        placeWord(clo + (-10),cla + (-8),4,8,"City Resources");
        drawBoundingBox(clo,cla,-10,-8,-4,-4);

        for(int i=0;i<resources.size();i++)
        {
            Resource* r = resources[i];
            int consumptionrate = city->getConsumptionRate(r->id);
            int productionrate = city->getProductionRate(r->id)-consumptionrate;
            int colsepar = 7;
            int j;
            for(j=0;j<consumptionrate;j++)
            {
                place((clo + (-10))*16-4+colsepar*j  ,(cla + (-7))*16-4+7*(i)  ,7,7,r->assetname);
            }

            for(;j<consumptionrate+productionrate;j++)
            {
                place((clo + (-10))*16-4+colsepar*(j+1)  ,(cla + (-7))*16-4+7*(i)  ,7,7,r->assetname);
            }

        }
    }

    placeWord(clo + (-10),cla + (-3),4,8,"Food Storage");
    drawBoundingBox(clo,cla,-10,-3,-4,9);

    for(int i=0;i<city->resources[0];i++)
        place((clo+(-10))*16-4+7*(i%10)  ,(cla+(-2))*16-4+7*(i/10)  ,7,7,"assets/assets/city/food.png");


    for(int i=0;i<city->buildings.size();i++)
    {
        placeWord(clo + (4),cla + (-10),4,8,city->buildings[i]->name, i*8);
        place((clo + (7))*16  ,(cla + (-10))*16+8*i  ,24,8,city->buildings[i]->assetname);
    }
    drawBoundingBox(clo,cla,4,-10,9,-1);

    placeWord(clo + (4),cla + (4),4,8,"Change");  // Row, Column
    if (city->productionQueue.size()>0)
    {
        BuildableFactory *bf = city->productionQueue.front();
        placeWord(clo + (7),cla + (4),4,8,bf->name);
    }
    else
    {
        placeWord(clo + (7),cla + (4),4,8,"Nothing");
    }
    drawBoundingBox(clo,cla,4,4,9,9);

    if (changeIsActive)
    {
        int i=0;
        for(auto it=city->buildable.begin();it!=city->buildable.end();it++)
        {
            BuildableFactory *bf = *it;
            placeWord(clo + (4),cla + (5),4,8,bf->name, (i)*8);
            if (selection>=0 && selection == i)
            {
                while (!city->productionQueue.empty()) city->productionQueue.pop();
                city->productionQueue.push(city->buildable[i]);
                changeIsActive = false;
                selection = -1;
            }
            i++;
        }
    }
    else
    {
        for(int i=0;i<city->resources[1];i++)
        {
            place((clo+(4))*16+7*(i%10)  ,(cla+(5))*16+7*(i/10)  ,7,7,"assets/assets/city/production.png");
        }
    }



    if (tileWorkingIsActive)
    {
        city->assignWorkingTile(clickedTile);
        tileWorkingIsActive = false;
    }


    for(int lats=-3;lats<=3;lats++)
        for(int lons=-3;lons<=3;lons++)
        {
            int la= cla + lats;
            int lo = clo + lons;

            if (city->occupied(lats,lons))
            {
                placeTile(lo,la,"assets/assets/general/occupied.png");
            }

            if (city->workingOn(lats,lons))
            {
                // @FIXME: Layout of resources per tile Needs to be better implemented


                // @NOTE: The algorithm seems to be like this.  Pick all the resources
                //   in this order:  Food, Production, Trade, Gold, Science, and whatever is left
                //   Then accumulate all the resources, divide them in two rows, and place them
                //   based on the allowed distance.  The maximimun is then 16 times 2, which is 32.
                std::vector<Resource*> resourcesToDisplay;

                for(int i=0;i<resources.size();i++)
                {
                    for(int j=0;j<map(la,lo).resource_production_rate[i];j++)
                    {
                        resourcesToDisplay.push_back(resources[i]);
                    }
                }

                //printf("Resources to display per tile: %d\n",resourcesToDisplay.size());
                int resperrow = ceil((float)resourcesToDisplay.size()/2.0);
                int colsepar = clipInt(16/resperrow,1,7);//3

                //printf("Resperrow and colsepar %d %d\n",resperrow,colsepar);

                for(int i=0;i<resourcesToDisplay.size();i++)
                {
                    //printf("Resource %d %s\n",i,resourcesToDisplay[i]->name);
                    place((lo)*16-4+colsepar*(i%resperrow)  ,(la)*16-4+7*(i/resperrow)  ,7,7,resourcesToDisplay[i]->assetname);
                }


                //for(int i=0;i<map(la,lo).resource_production_rate[0];i++)
                //    place((lo)*16-4+7*i  ,(la)*16-4  ,7,7,resources[0]->assetname);

                //int i=0;
                //for(i=0;i<map(la,lo).resource_production_rate[1];i++)
                //    place((lo)*16-4+7*i  ,(la)*16-4+7  ,7,7,resources[1]->assetname);

                //for(int j=i;j<map(la,lo).resource_production_rate[2]+i;j++)
                //    place((lo)*16-4+7*j  ,(la)*16-4+7  ,7,7,resources[2]->assetname);

                //place((lo)*16-4  ,(la)*16-4  ,7,7,"assets/assets/city/food.png");
                //place((lo)*16-4+7,(la)*16-4  ,7,7,"assets/assets/city/food.png");
                //place((lo)*16-4  ,(la)*16-4+7,7,7,"assets/assets/city/production.png");
                //place((lo)*16-4+7,(la)*16-4+7,7,7,"assets/assets/city/trade.png");
            }
        }

}