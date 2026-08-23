#include <unordered_map>
#include <vector>

#include "openglutils.h"
#include "math/yamathutil.h"
#include "lodepng.h"
#include "font/FontsBitmap.h"
#include "font/DrawFonts.h"
#include "map.h"
#include "resources.h"
#include "City.h"
#include "Faction.h"
#include "units/Unit.h"
#include "engine.h"
#include "usercontrols.h"
#include "tiles.h"
#include "cityscreenui.h"

extern float cx;
extern float cy;

extern Map map;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Controller controller;

// Units currently standing on city's tile, in a stable order shared by drawCityScreen (to
// list them) and clickOnCityScreen (to map a clicked row back to the same unit).
std::vector<Unit*> getUnitsAtCity(City* city)
{
    std::vector<Unit*> stationed;
    for (auto& [k,u] : units)
    {
        if (u->latitude == city->latitude && u->longitude == city->longitude)
            stationed.push_back(u);
    }
    return stationed;
}

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
// @FIXME: Improve this please.....
bool changeIsActive = false;
int selection = -1;
int selectionOffset = 0;

// Scroll offset for the "Units" box, same mechanism as selectionOffset above (0 = top of
// the list, decreasing reveals later units) -- see drawCityScreen/clickOnCityScreen.
int unitsOffset = 0;
const int UNITS_BOX_ROWS = 5;

// Scroll offset for the "Commodities Storage" box, same mechanism as unitsOffset above.
int commoditiesStorageOffset = 0;
const int COMMODITIES_STORAGE_ROWS = 5;

coordinate clickedTile(0,0);
bool tileWorkingIsActive = false;

void clickOnCityScreen(int lat, int lon, int lat2, int lon2)
{
    printf("Click on City Screen %d,%d - %d, %d\n",lat,lon, lat2, lon2);
    if ((lat==4 && lon==5) || (lat==4 && lon==4))
    {
        printf("Change\n"); // Row, Column
        changeIsActive = true;
    }

    if (changeIsActive)
    {
        if (lat2==10 && lon2==18)
        {
            // Move up
            selectionOffset++;
        } else
        if (lat2==18 && lon2==18)
        {
            // Move down
            selectionOffset--;
        } else if (lon2!=18)
        {
            // lon2==18 is the up/down arrows' own column (they sit on the SAME row as the
            // list's first/last item, lat2==10/18 respectively -- see the comment above).  A
            // click that misses the exact arrow row by a pixel (very plausible: the arrow
            // icon is only ~8px tall) but still lands in that column must NOT fall through to
            // selecting whatever item happens to share that row, or the production queue gets
            // silently changed instead of the list just scrolling.
            selection = lat2 - 10 + selectionOffset*(-1);
            printf("Selection %d\n",selection);
        }
    }

    if ((lat>-3 || lat<3) && (lon>-3 || lon<3))
    {
        printf("Tile Working....\n");
        tileWorkingIsActive = true;
        clickedTile = coordinate(lat,lon);
    }

    // "Units" box, directly below the map (drawCityScreen): one stationed unit per row,
    // rows 5..9 (row 4 is the box's own label).  Same up/down arrow mechanism as the
    // Change (buildable) list above -- same look and feel, but at the Units box's own
    // column (endright=3, vs the Change box's endright=9): the row values are identical
    // between the two boxes (both start at row 5), which is why lat2 stays 10/18 here too;
    // only lon2 differs, since lon2/lat2 come from the SAME formula each Change arrow
    // proved out (drawCityScreen: (box_coordinate)*16 in pixels -> lat2/lon2 = pixels/8, so
    // column 3 -> 3*16/8 = 6, matching the Change arrows' column 9 -> 9*16/8 = 18).
    if (lat2==10 && lon2==6)
    {
        // Move up
        unitsOffset++;
    } else
    if (lat2==18 && lon2==6)
    {
        // Move down
        unitsOffset--;
    }

    // "Commodities Storage" box, bottom-left (drawCityScreen): same up/down arrow mechanism
    // as the Units box above, at that box's own column (endright=-4, on the LEFT of the map
    // this time) -- same formula, lon2 = endright*16/8 = -4*16/8 = -8 (negative since the
    // box sits left of center).
    if (lat2==10 && lon2==-8)
    {
        // Move up
        commoditiesStorageOffset++;
    } else
    if (lat2==18 && lon2==-8)
    {
        // Move down
        commoditiesStorageOffset--;
    }

    // A city only opens its screen for its owner (usercontrols.cpp), so activating a unit
    // found here is always the player's.
    // cities.find (not cities[...]): operator[] on a missing key would INSERT A NULL entry.
    auto cityIt = cities.find(controller.cityid);
    if (cityIt != cities.end() && lat>=5 && lat<=9 && lon>=-3 && lon<=3)
    {
        std::vector<Unit*> stationed = getUnitsAtCity(cityIt->second);
        int loc = lat - 5;
        int idx = loc - unitsOffset;
        if (idx >= 0 && idx < (int)stationed.size())
        {
            activateUnit(stationed[idx]);
        }
    }

}

void getFoodStorageLayout(int pop, int &itemsPerRow, int &colsepar)
{
    // Same box width as City Resources/City Commodities above (cols -10..-4, 6 tiles), whose
    // colsepar formula's own comment says "16 resources fit with a colsepar of 7" -- so 16 is
    // the row's natural (uncompressed) capacity here too. Picking itemsPerRow only off
    // ceil(thresshold/rows) (as before) starves small thresholds down to far fewer per row
    // (e.g. 8 at pop=1) even though the row has room for twice that -- so take whichever is
    // larger, the natural capacity or what a large thresshold actually needs.
    const int NATURAL_ITEMS_PER_ROW = 16;

    int foodStorageRows = ((3)-(-3))*16/7;
    int foodStorageWidth = ((-4)-(-10))*16;
    int foodThresshold = getPopulationThresshold(pop);

    itemsPerRow = NATURAL_ITEMS_PER_ROW;
    int neededPerRow = (int)ceil((float)foodThresshold/(float)foodStorageRows);
    if (neededPerRow > itemsPerRow)
        itemsPerRow = neededPerRow;

    // Spacing is measured between an icon's LEFT edges, so the last icon's right edge sits
    // at colsepar*(itemsPerRow-1)+7 (icon width) -- that must stay <= the box width, not
    // colsepar*itemsPerRow, or a tightly packed row overruns the box by up to 7px.
    if (itemsPerRow<=1)
        colsepar = 7;
    else
        colsepar = clipInt( (int)floor((float)(foodStorageWidth-7)/(float)(itemsPerRow-1)), -7, 7);
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

    // Units stationed on the city's tile, just below the map box: clicking a city no
    // longer activates them directly (usercontrols.cpp), this box is how the player picks
    // one instead (clickOnCityScreen maps a clicked row back to the same list below).
    {
        placeWord(clo + (-3),cla + (4),4,8,"Units");
        drawBoundingBox(clo,cla,-3,4,3,9);

        std::vector<Unit*> stationed = getUnitsAtCity(city);

        // Same scrolling mechanism as the Change (buildable) list below: loc is the
        // on-screen row, i is the absolute index into stationed; items scrolled above
        // the box (loc<0) are skipped, drawing stops once the last visible row is filled.
        int i=0;
        for(auto it=stationed.begin();it!=stationed.end();it++)
        {
            int loc = i+unitsOffset;
            if (loc<0) {i++;continue;}

            Unit* u = *it;
            Faction* f = factions[u->faction];
            // placeTile (not place): units are tinted to their faction's color, same as
            // placeThisUnit does on the map (Unit::draw()) -- plain place() draws the raw
            // asset untinted, which is what looked wrong here.
            placeTile(cla + (5+loc), clo + (-3), 16, u->getAssetName(), f->red, f->green, f->blue);

            // Same status overlays as the map (Unit::draw()), through the same
            // getOverlayAssets() so any status added there shows up here too.
            for (const char* overlay : u->getOverlayAssets())
            {
                placeTile(cla + (5+loc), clo + (-3), 16, overlay, f->red, f->green, f->blue);
            }

            placeWord(clo + (-1),cla + (5+loc),4,8,u->name);

            i++;
            if (loc==UNITS_BOX_ROWS-1) break;
        }

        // Up/down arrows, same asset/position style as the Change list's (endright=3
        // here vs endright=9 there): only shown, and only meaningful to click, once there
        // are more units than fit.
        if ((int)stationed.size()>UNITS_BOX_ROWS)
        {
            place((clo+(3))*16,(cla+(5))*16 ,7,7,"assets/assets/cursor/up.png");
            place((clo+(3))*16,(cla+(9))*16 ,7,7,"assets/assets/cursor/down.png");

            if (unitsOffset>0) unitsOffset=0;
            int max = ((int)stationed.size())-UNITS_BOX_ROWS;
            if (unitsOffset<-max)
                unitsOffset=(stationed.size()-UNITS_BOX_ROWS)*(-1);
        }
        else
        {
            // Nothing to scroll: keep the offset reset so a later overflow starts fresh.
            unitsOffset = 0;
        }
    }

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

        // In the box 16 resources fit with a colsepar of 7.
        for(int i=0;i<resources.size();i++)
        {
            Resource* r = resources[i];
            int consumptionrate = city->getConsumptionRate(r->id);
            int productionrate = city->getProductionRate(r->id)-consumptionrate;

            // @FIXME: This works but it is not very good.
            // @TODO: Pick an icon to highlight the situation where resources are not enough to cover the consumption rate.  This is a very important situation and should be highlighted.
            // @TODO: Allow clicking on the resources to see the number of resources (when there are a lot is going to be hard to count)
            int colsepar = clipInt( floor(7*(16.0/((consumptionrate+productionrate)))),1,7);
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

    {
        // Per-turn commodity gather rate (task #13), directly above the map box (same
        // columns, -3..3). Same "pack together" row style as City Resources above, but one
        // row per commodity actually being produced THIS TURN (rate<=0 is skipped) -- with
        // up to 21 possible commodity types, a fixed row per type would never fit.
        // Starts at row -8 (not -10), same as City Resources: rows -10..-9 stay clear across
        // the WHOLE top of the screen for the population icons (placed at row -9, growing
        // rightward from column -10 as pop increases) to have room, instead of running into
        // this box's top border.
        placeWord(clo + (-3),cla + (-8),4,8,"City Commodities");
        drawBoundingBox(clo,cla,-3,-8,3,-4);

        int i=0;
        for (int commodity_id : ALL_COMMODITIES)
        {
            int rate = city->getCommodityProductionRate(commodity_id);
            if (rate<=0) continue;

            int colsepar = clipInt( floor(7*(16.0/rate)),1,7);
            for(int j=0;j<rate;j++)
            {
                place((clo + (-3))*16-4+colsepar*j  ,(cla + (-7))*16-4+7*(i)  ,7,7,tiles[commodity_id].c_str());
            }
            i++;
        }
    }

    placeWord(clo + (-10),cla + (-3),4,8,"Food Storage");
    drawBoundingBox(clo,cla,-10,-3,-4,3);

    // The box shrank (rows -3..3, was -3..9) to make room for Commodities Storage below it.
    // Sized to the food needed to grow population by one (City.cpp getPopulationThresshold),
    // not the current stock -- resources[0] can climb all the way up to that thresshold
    // (bunmei.cpp endOfYear) right before the city grows, so the grid must already fit that
    // many icons, not just whatever's stored right now. Rows stay fixed at the box's natural
    // 7px row height; the column spacing (colsepar) is squeezed instead -- same technique as
    // City Resources/Commodities above, but allowed below their [1,7] floor since the
    // thresshold (100*pop) grows unbounded with population.
    int foodItemsPerRow, foodColsepar;
    getFoodStorageLayout(city->pop, foodItemsPerRow, foodColsepar);

    for(int i=0;i<city->resources[0];i++)
        place((clo+(-10))*16-4+foodColsepar*(i%foodItemsPerRow)  ,(cla+(-2))*16-4+7*(i/foodItemsPerRow)  ,7,7,"assets/assets/city/food.png");

    {
        // Commodity stockpile (task #13): a scrollable list (same mechanism as the Units
        // box), one row per commodity actually in storage. The trailing icon is a "load"
        // placeholder for a later Wagon/Ship transfer feature -- not wired to a click yet.
        placeWord(clo + (-10),cla + (4),4,8,"Commodities Storage");
        drawBoundingBox(clo,cla,-10,4,-4,9);

        std::vector<int> stocked;
        for (int commodity_id : ALL_COMMODITIES)
            if (city->commodities[commodity_id] > 0)
                stocked.push_back(commodity_id);

        int i=0;
        for(auto it=stocked.begin();it!=stocked.end();it++)
        {
            int loc = i+commoditiesStorageOffset;
            if (loc<0) {i++;continue;}

            int commodity_id = *it;
            // placeTile(x,y,...) here is the UNTINTED overload: x is the COLUMN (lon), y is
            // the ROW (lat) -- opposite argument order from the tinted placeTile(lat,lon,...)
            // the Units box above uses.
            placeTile(clo + (-10), cla + (5+loc), 7, tiles[commodity_id].c_str());

            char countstr[16];
            sprintf(countstr,"%d",city->commodities[commodity_id]);
            placeWord(clo + (-9),cla + (5+loc),4,8,countstr);

            placeTile(clo + (-5), cla + (5+loc), 7, "assets/assets/cursor/right.png");

            i++;
            if (loc==COMMODITIES_STORAGE_ROWS-1) break;
        }

        if ((int)stocked.size()>COMMODITIES_STORAGE_ROWS)
        {
            place((clo+(-4))*16,(cla+(5))*16 ,7,7,"assets/assets/cursor/up.png");
            place((clo+(-4))*16,(cla+(9))*16 ,7,7,"assets/assets/cursor/down.png");

            if (commoditiesStorageOffset>0) commoditiesStorageOffset=0;
            int max = ((int)stocked.size())-COMMODITIES_STORAGE_ROWS;
            if (commoditiesStorageOffset<-max)
                commoditiesStorageOffset=(stocked.size()-COMMODITIES_STORAGE_ROWS)*(-1);
        }
        else
        {
            commoditiesStorageOffset = 0;
        }
    }


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
        int slots = 9;
        for(auto it=city->buildable.begin();it!=city->buildable.end();it++)
        {
            int loc = i+selectionOffset;
            if (loc<0) {i++;continue;}
            //printf("First element: %d offset %d\n", i, selectionOffset);
            BuildableFactory *bf = *it;
            placeWord(clo + (4),cla + (5),4,8,bf->name, (loc)*8);
            if (selection>=0 && selection == i)
            {
                while (!city->productionQueue.empty()) city->productionQueue.pop();
                city->productionQueue.push(city->buildable[i]);
                changeIsActive = false;
                selection = -1;
            }
            i++;
            if (loc==slots-1) break;
        }
        if (city->buildable.size()>slots)
        {
            place((clo+(9))*16,(cla+(5))*16 ,7,7,"assets/assets/cursor/up.png");
            place((clo+(9))*16,(cla+(9))*16 ,7,7,"assets/assets/cursor/down.png");
        }

        if (selectionOffset>0) selectionOffset=0;
        int max = ((int)city->buildable.size())-slots;
        if (selectionOffset<-max) 
            selectionOffset=(city->buildable.size()-slots)*(-1);

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
                    float resourceProductionRate = map(la,lo).getResourceProductionRate(i);

                    std::vector<int> resourceProductionRateVector;

                    // FOOD, PRODUCTION, TRADE, GOLD, SCIENCE, CULTURE
                    //resourceProductionRateVector.push_back();



                    for(int j=0;j<resourceProductionRate;j++)
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