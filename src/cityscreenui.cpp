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
#include "units/Transport.h"
#include "engine.h"
#include "usercontrols.h"
#include "tiles.h"
#include "coordinator.h"
#include "cityscreenui.h"
#include "dee.h"

extern float cx;
extern float cy;

extern Map map;
extern Tiles tiles;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Controller controller;
extern Coordinator coordinator;
extern DependencyEvaluationEngine dee;

std::unordered_map<int, std::string> coreresources;

void initCoreResources()
{
    coreresources[FOOD] = "assets/assets/city/food.png";
    coreresources[SHIELDS] = "assets/assets/city/production.png";
    coreresources[TRADE] = "assets/assets/city/trade.png";
    coreresources[COINS] = "assets/assets/city/gold.png";
    coreresources[SCIENCE] = "assets/assets/city/bulb.png";
    coreresources[CULTURE] = "assets/assets/city/culture.png";
}


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

// Same reasoning as getUnitsAtCity above: shared by drawCityScreen (to list the
// "Resource Storage" box) and clickOnCityScreen (to map a clicked row's "load" arrow back
// to the same resource id), so both always agree on row-to-resource order. Stocked
// commodities come first (ALL_COMMODITIES order), then stocked mfg goods (ALL_MFG_GOODS
// order) -- the two id ranges never overlap (COMMODITIES 0x2xx, MFGOODS 0x3xx), so a plain
// `id >= rum` tells the two apart everywhere this list is consumed.
std::vector<int> getStockedResources(City* city)
{
    std::vector<int> stocked;
    for (int commodity_id : ALL_COMMODITIES)
        if (city->commodities[commodity_id] > 0)
            stocked.push_back(commodity_id);
    for (int mfggood_id : ALL_MFG_GOODS)
        if (city->mfggoods[mfggood_id] > 0)
            stocked.push_back(mfggood_id);
    return stocked;
}

// How many units of a stocked resource id the city holds -- commodities and mfg goods live
// in separate maps, split by the same `id >= rum` test getStockedResources documents.
static int stockedResourceAmount(City* city, int id)
{
    return id >= rum ? city->mfggoods[id] : city->commodities[id];
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

// Scroll offset for the "Resource Storage" box, same mechanism as unitsOffset above.
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
        if (!changeIsActive)
        {
            // Opening the Change list: (re)populate city->buildable, which otherwise stays
            // empty from when the city was founded (BuildCityOrder).
            CommandOrder co;
            co.command = Command::PopulateBuildableOrder;
            co.parameters.cityid = controller.cityid;
            coordinator.push(co);
        }
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

    // "Resource Storage" box, bottom-left (drawCityScreen): same up/down arrow mechanism
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

            // Cargo slot icons (drawCityScreen's own comment on this box has the lon2
            // derivation): slot s sits at lon2 == s-4, one per capacity() slot -- slot 0 at
            // lon2==-4 up to slot 5 at lon2==1 for a Galleon. Clicking a loaded slot unloads
            // that resource back into this city; an empty slot is a no-op (the command
            // handler finds nothing at that resource id and does nothing).
            if (Transport* transport = dynamic_cast<Transport*>(stationed[idx]))
            {
                int slot = lon2 + 4;
                if (slot >= 0 && slot < transport->capacity())
                {
                    std::vector<Shippable*> cargo = transport->getCargo();
                    if (slot < (int)cargo.size())
                    {
                        CommandOrder co;
                        co.command = Command::UnloadCargoOrder;
                        co.parameters.spawnid = stationed[idx]->id;
                        co.parameters.factionid = stationed[idx]->faction;
                        co.parameters.cityid = controller.cityid;
                        co.parameters.resourceid = cargo[slot]->getId();
                        coordinator.push(co);
                    }
                }
            }
        }
    }

    // "Resource Storage" box's per-row "load" arrow (cursor/right.png, column -5,
    // drawCityScreen): loads up to 100 units of that row's resource (commodity or mfg good)
    // onto whichever unit is currently active/selected (coordinator.a_u_id) -- click a
    // Transport in the Units box above first, same as any other unit-targeted action in this
    // UI. No fine resolution needed here (unlike the cargo slots above): the arrow is the
    // only thing at this column, so a plain coarse lon check is enough, same as the Units
    // box row check above.
    if (cityIt != cities.end() && lat>=5 && lat<=9 && lon==-5)
    {
        std::vector<int> stocked = getStockedResources(cityIt->second);
        int loc = lat - 5;
        int idx = loc - commoditiesStorageOffset;
        if (idx >= 0 && idx < (int)stocked.size())
        {
            auto unitIt = units.find(coordinator.a_u_id);
            if (unitIt != units.end() && dynamic_cast<Transport*>(unitIt->second) != nullptr)
            {
                CommandOrder co;
                co.command = Command::LoadCargoOrder;
                co.parameters.spawnid = unitIt->second->id;
                co.parameters.factionid = unitIt->second->faction;
                co.parameters.cityid = controller.cityid;
                co.parameters.resourceid = stocked[idx];
                coordinator.push(co);
            }
        }
    }

}

void getFoodStorageLayout(int pop, int &itemsPerRow, float &colsepar)
{
    // Same box width as City Resources/City Commodities above (cols -10..-4, 6 tiles).
    // itemsPerRow is the TIGHTEST fit that spreads the whole thresshold across every row
    // the box has (foodStorageRows) -- so the grid always uses the box's full height,
    // instead of stopping partway down whenever a fixed per-row minimum (this used to be a
    // "natural" 16-items floor) needed fewer rows than the box actually has.
    //
    // colsepar is a FLOAT on purpose: the call site applies it per-icon via
    // round(colsepar*j), not a single truncated division reused for the whole row. A plain
    // int colsepar (floor()'d once) loses up to itemsPerRow-2 units of width to rounding,
    // visibly stopping short of the box's right edge for any row with more than a handful
    // of items -- per-icon rounding instead lands the LAST icon exactly on the box's edge,
    // using the full width for any itemsPerRow.
    int foodStorageRows = ((3)-(-3))*16/7;
    int foodStorageWidth = ((-4)-(-10))*16;
    int foodThresshold = getPopulationThresshold(pop);

    itemsPerRow = (int)ceil((float)foodThresshold/(float)foodStorageRows);
    if (itemsPerRow<1) itemsPerRow = 1;

    // Spacing is measured between an icon's LEFT edges, so the last icon's right edge sits
    // at colsepar*(itemsPerRow-1)+7 (icon width) -- that must stay <= the box width, not
    // colsepar*itemsPerRow, or a tightly packed row overruns the box by up to 7px.
    if (itemsPerRow<=1)
        colsepar = 7.0f;
    else
        colsepar = (float)(foodStorageWidth-7)/(float)(itemsPerRow-1);
}

// One "Units" box row: faction-tinted unit icon, status overlays, name, and (for a
// Transport) one box.png cargo slot per capacity() slot. Row `loc` -> lat cla+(5+loc).
// Factored out of drawCityScreen's Units box so the commerce screen's "port" box can draw
// the trading Transport identically.
void drawUnitsBoxRow(int cla, int clo, Unit* u, int loc)
{
    Faction* f = factions[u->faction];
    Transport* transport = dynamic_cast<Transport*>(u);

    // placeTile (not place): units are tinted to their faction's color, same as
    // placeThisUnit does on the map (Unit::draw()) -- plain place() draws the raw asset
    // untinted, which is what looked wrong here.
    placeTile(cla + (5+loc), clo + (-3), 16, u->getAssetName(), f->red, f->green, f->blue);

    // Same status overlays as the map (Unit::draw()), through the same getOverlayAssets()
    // so any status added there shows up here too.
    for (const char* overlay : u->getOverlayAssets())
        placeTile(cla + (5+loc), clo + (-3), 16, overlay, f->red, f->green, f->blue);

    // A big Transport (Galleon, capacity 6) needs more room for its cargo slots than the
    // 2-slot Trireme/Wagon, so its name is pushed one column right (past the last slot)
    // instead of sitting at column -1.
    bool wideCargo = transport != nullptr && transport->capacity() > 2;
    placeWord(clo + (wideCargo ? 1 : -1), cla + (5+loc), 4, 8, u->name);

    // Cargo slots: one small box.png per cargo slot the Transport HOLDS IN TOTAL
    // (capacity(), not how many are free), starting at raw pixel -32 (just right of the
    // unit icon at column -3). A loaded slot draws the resource's own icon first, then
    // box.png on top of it (box.png's border is opaque, its middle transparent, so it reads
    // as a little frame around the resource icon). Slot x sits at HALF-column offsets
    // (x = slot*8 - 32), so each slot lands on its own value of the fine click grid
    // (lon2 = column*2) -- slot 0 -> lon2=-4 ... slot 5 -> lon2=1 -- letting the click
    // handler tell all 6 apart via lon2 alone.
    if (transport != nullptr)
    {
        std::vector<Shippable*> cargo = transport->getCargo();
        int y = (cla + (5+loc))*16;
        for (int slot=0; slot<transport->capacity(); slot++)
        {
            int x = clo*16 + slot*8-32;
            if (slot < (int)cargo.size())
                place(x,y,7,7,tiles[cargo[slot]->getId()].c_str());
            place(x,y,7,7,"assets/assets/city/box.png");
        }
    }
}

// The bottom-left "Resource Storage" box: one scrollable row per stocked resource, an icon
// strip of 1 icon / 10 units (floored, capped at 30 for the 300-per-city ceiling; the 30
// slots span the box width up to a 16px gap for the count digits), the exact count, then
// `arrowAsset` pinned at column -5, plus up/down scroll arrows once the list overflows.
// `scrollOffset` is the caller's own offset (clamped here). Factored out of drawCityScreen
// so the commerce screen's "for sale" box renders identically (with a "buy" arrow).
void drawResourceStorageBox(int cla, int clo, City* city, int& scrollOffset,
                            const char* label, const char* arrowAsset)
{
    placeWord(clo + (-10), cla + (4), 4, 8, label);
    drawBoundingBox(clo, cla, -10, 4, -4, 9);

    std::vector<int> stocked = getStockedResources(city);

    const int   RESOURCE_STRIP_SLOTS = 30;
    const int   stripLeftPx  = (clo + (-10))*16;
    const int   stripRightPx = (clo + (-5))*16 - 16;
    const float resourceStripStep = (float)(stripRightPx - stripLeftPx - 7) / (float)(RESOURCE_STRIP_SLOTS - 1);

    int i=0;
    for(auto it=stocked.begin();it!=stocked.end();it++)
    {
        int loc = i+scrollOffset;
        if (loc<0) {i++;continue;}

        int resource_id = *it;
        int amount = stockedResourceAmount(city, resource_id);
        int rowY = (cla + (5+loc))*16;

        int icons = amount/10;
        if (icons > RESOURCE_STRIP_SLOTS) icons = RESOURCE_STRIP_SLOTS;
        for (int j=0;j<icons;j++)
            place(stripLeftPx + (int)round(resourceStripStep*j)  ,rowY  ,7,7,tiles[resource_id].c_str());

        // Count digits immediately after the last drawn icon (or at the left edge when the
        // amount is below one full icon). placeWord multiplies its x by 16, so a fractional
        // column places it at an exact pixel.
        int countPx = stripLeftPx + (icons>0 ? (int)round(resourceStripStep*(icons-1)) + 7 + 2 : 0);
        char countstr[16];
        sprintf(countstr,"%d",amount);
        placeWord(countPx/16.0f,cla + (5+loc),4,8,countstr);

        placeTile(clo + (-5), cla + (5+loc), 7, arrowAsset);

        i++;
        if (loc==COMMODITIES_STORAGE_ROWS-1) break;
    }

    if ((int)stocked.size()>COMMODITIES_STORAGE_ROWS)
    {
        place((clo+(-4))*16,(cla+(5))*16 ,7,7,"assets/assets/cursor/up.png");
        place((clo+(-4))*16,(cla+(9))*16 ,7,7,"assets/assets/cursor/down.png");

        if (scrollOffset>0) scrollOffset=0;
        int max = ((int)stocked.size())-COMMODITIES_STORAGE_ROWS;
        if (scrollOffset<-max)
            scrollOffset=(stocked.size()-COMMODITIES_STORAGE_ROWS)*(-1);
    }
    else
    {
        scrollOffset = 0;
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

            drawUnitsBoxRow(cla, clo, *it, loc);

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
        for (int i=0;i<sizeof(ALL_CORE_RESOURCES)/sizeof(int);i++)
        {
            int r = ALL_CORE_RESOURCES[i];
            int consumptionrate = city->getConsumptionRate(r);
            int productionrate = city->getProductionRate(r)-consumptionrate;

            // @FIXME: This works but it is not very good.
            // @TODO: Pick an icon to highlight the situation where resources are not enough to cover the consumption rate.  This is a very important situation and should be highlighted.
            // @TODO: Allow clicking on the resources to see the number of resources (when there are a lot is going to be hard to count)
            int colsepar = clipInt( floor(7*(16.0/((consumptionrate+productionrate)))),1,7);
            int j;


            for(j=0;j<consumptionrate;j++)
            {
                place((clo + (-10))*16-4+colsepar*j  ,(cla + (-7))*16-4+7*(i)  ,7,7,coreresources[r].c_str());
            }

            for(;j<consumptionrate+productionrate;j++)
            {
                place((clo + (-10))*16-4+colsepar*(j+1)  ,(cla + (-7))*16-4+7*(i)  ,7,7,coreresources[r].c_str());
            }

        }

    }

    {
        // Per-turn production (task #13, extended for mfg goods), directly above the map box
        // (columns -3..3). Two groups, "pack together" row style like City Resources above,
        // one row each -- rows -10..-9 stay clear across the WHOLE top of the screen for the
        // population icons (row -9, growing right from column -10), so this box starts at -8:
        //   1. commodities the city GATHERS this turn from special resources in range
        //      (getCommodityProductionRate > 0).
        //   2. mfg goods MANUFACTURED this turn by the city's buildings: drawn as
        //      "<consumed commodity icons> <slash icon> <produced mfg good icons>", where the
        //      slash icon is slashok.png when the city stocks every input this turn and
        //      slashnook.png when it is short one (nothing is produced then -- the same
        //      `enough` test engine.cpp operateCityBuildings() gates production on).
        placeWord(clo + (-3),cla + (-8),4,8,"City Production");
        drawBoundingBox(clo,cla,-3,-8,3,-4);

        // Row icons start a half-tile inside the box's left border (was -4, which drew them
        // on top of the border tile).
        const int rowLeftPx = (clo + (-3))*16 + 8;

        int i=0;
        for (int commodity_id : ALL_COMMODITIES)
        {
            int rate = city->getCommodityProductionRate(commodity_id);
            if (rate<=0) continue;

            int colsepar = clipInt( floor(7*(16.0/rate)),1,7);
            for(int j=0;j<rate;j++)
            {
                place(rowLeftPx+colsepar*j  ,(cla + (-7))*16-4+7*(i)  ,7,7,tiles[commodity_id].c_str());
            }
            i++;
        }

        for (Building* b : city->buildings)
        {
            for (int mfg_id : ALL_MFG_GOODS)
            {
                int prate = b->getProductionRate(mfg_id);
                if (prate<=0) continue;

                int rowY = (cla + (-7))*16-4+7*(i);
                int x = rowLeftPx;

                // Consumed commodities, one icon per unit of consumption rate. `enough` is
                // the same check operateCityBuildings() gates production on.
                bool enough = true;
                for (int cmd_id : ALL_COMMODITIES)
                {
                    int crate = b->getConsumptionRate(cmd_id);
                    if (crate<=0) continue;
                    if (city->commodities[cmd_id] < crate) enough = false;
                    for (int j=0;j<crate;j++, x+=7)
                        place(x  ,rowY  ,7,7,tiles[cmd_id].c_str());
                }

                // The "/" divider icon: slashok when every input is covered, slashnook on a
                // shortage (the same `enough` test operateCityBuildings() gates production on).
                place(x  ,rowY  ,7,7, enough ? "assets/assets/city/slashok.png"
                                             : "assets/assets/city/slashnook.png");
                x += 7;

                for (int j=0;j<prate;j++, x+=7)
                    place(x  ,rowY  ,7,7,tiles[mfg_id].c_str());

                i++;
            }
        }
    }

    placeWord(clo + (-10),cla + (-3),4,8,"Food Storage");
    drawBoundingBox(clo,cla,-10,-3,-4,3);

    // The box shrank (rows -3..3, was -3..9) to make room for Commodities Storage below it.
    // Sized to the food needed to grow population by one (City.cpp getPopulationThresshold),
    // not the current stock -- resources[0] can climb all the way up to that thresshold
    // (bunmei.cpp endOfYear) right before the city grows, so the grid must already fit that
    // many icons, not just whatever's stored right now. itemsPerRow/colsepar (see
    // getFoodStorageLayout) are picked to use the box's full height AND full width for any
    // population -- colsepar is a float applied per-icon with round() below (not truncated
    // once for the whole row), so the row's last icon lands exactly on the box's edge.
    int foodItemsPerRow; float foodColsepar;
    getFoodStorageLayout(city->pop, foodItemsPerRow, foodColsepar);

    for(int i=0;i<city->coreresources[FOOD];i++)
        place((clo+(-10))*16-4+(int)round(foodColsepar*(i%foodItemsPerRow))  ,(cla+(-2))*16-4+7*(i/foodItemsPerRow)  ,7,7,"assets/assets/city/food.png");

    // Blue line marking the Granary's reserve (task #26): once HALF_POPULATION_CODE is
    // active for this city (a Granary has been built, see Granary.cpp/bunmei.cpp endOfYear),
    // half of the food thresshold needed to grow (City.cpp getPopulationThresshold) is kept
    // instead of lost (bunmei.cpp endOfYear applies this same half, against the SAME --
    // already post-growth -- city->pop this line reads, so right after a growth tick the
    // kept reserve lines up exactly with this row). The line sits at that half-way point in
    // the same icon grid the food above is drawn in (icons fill top-down as resources[0]
    // grows), so it splits the box into the kept reserve (above the line) and any food
    // accumulated since the last growth (below it).
    if (dee.verifyDep(cityContext(city->id), HALF_POPULATION_CODE))
    {
        int halfThresshold = getPopulationThresshold(city->pop)/2;
        int row = halfThresshold/foodItemsPerRow;
        // x is measured the same way place() measures icon x: the CENTER of the shape, not
        // its left edge. The icon ROW's own visual footprint runs from icon 0's center minus
        // half an icon width to the last icon's center plus half an icon width -- lineSpan is
        // the distance between those two centers, so the line's center must sit lineSpan/2
        // past the row's start (icon 0's center), not lineWidth/2 (that overshoots by half an
        // icon width, landing the line consistently to the right of the icons).
        int lineSpan = (int)round(foodColsepar*(foodItemsPerRow-1));
        int lineWidth = lineSpan+7;
        placeColorBar((clo+(-10))*16-4+lineSpan/2  ,(cla+(-2))*16-4+7*row-4  ,lineWidth,2,0.0f,0.0f,1.0f);
    }

    // Resource stockpile (task #13, extended for mfg goods): the bottom-left box, one
    // scrollable row per stocked commodity/mfg good, an icon strip of 1 icon / 10 units, the
    // count, and a per-row cursor/right.png arrow that loads that resource onto the active
    // Transport (clickOnCityScreen -> Command::LoadCargoOrder). Shared with the commerce
    // screen (see drawResourceStorageBox above).
    drawResourceStorageBox(cla, clo, city, commoditiesStorageOffset,
                           "Resource Storage", "assets/assets/cursor/right.png");


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
        // When the list is SHORTER than the box (size<slots), size-slots is negative -- floor
        // it at 0 so the clamp below can't flip sign and push a short list toward the bottom
        // of the box instead of leaving it top-justified (issue2.png: a short list -- Palace/
        // Scout/Settler/Worker/Warrior -- appeared shifted down with a blank gap above it).
        int max = ((int)city->buildable.size())-slots;
        if (max<0) max=0;
        if (selectionOffset<-max)
            selectionOffset=-max;

    }
    else
    {
        for(int i=0;i<city->coreresources[SHIELDS];i++)
        {
            place((clo+(4))*16+7*(i%10)  ,(cla+(5))*16+7*(i/10)  ,7,7,"assets/assets/city/production.png");
        }
    }



    if (tileWorkingIsActive)
    {
        // Command pattern: the actual assign/deassign toggle happens in
        // engine.cpp:processCommandOrders() (Command::AssignWorkTileOrder), not here.
        CommandOrder co;
        co.command = Command::AssignWorkTileOrder;
        co.parameters.cityid = city->id;
        co.parameters.latitude = clickedTile.lat;
        co.parameters.longitude = clickedTile.lon;
        coordinator.push(co);

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
                std::vector<int> resourcesToDisplay;

                for(int i=0;i<sizeof(ALL_CORE_RESOURCES)/sizeof(int);i++)
                {
                    float resourceProductionRate = map(la,lo).getResourceProductionRate(i);

                    std::vector<int> resourceProductionRateVector;

                    // FOOD, PRODUCTION, TRADE, GOLD, SCIENCE, CULTURE
                    //resourceProductionRateVector.push_back();



                    for(int j=0;j<resourceProductionRate;j++)
                    {
                        resourcesToDisplay.push_back(i);
                    }
                }

                //printf("Resources to display per tile: %d\n",resourcesToDisplay.size());
                int resperrow = ceil((float)resourcesToDisplay.size()/2.0);
                int colsepar = clipInt(16/resperrow,1,7);//3

                //printf("Resperrow and colsepar %d %d\n",resperrow,colsepar);

                for(int i=0;i<resourcesToDisplay.size();i++)
                {
                    //printf("Resource %d %s\n",i,resourcesToDisplay[i]->name);
                    place((lo)*16-4+colsepar*(i%resperrow)  ,(la)*16-4+7*(i/resperrow)  ,7,7,coreresources[resourcesToDisplay[i]].c_str());
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