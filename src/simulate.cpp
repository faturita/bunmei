#include "mapmodel.h"
#include "tiles.h"
#include "resources.h"
#include "Faction.h"
#include "City.h"
#include "units/Unit.h"

typedef std::unordered_map<int, City*> Cities;
typedef std::unordered_map<int, Unit*> Units;

void initMap(Map &map, Tiles tiles)
{
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }    
}

void initFactions(std::vector<Faction*> &factions, Map &map, std::unordered_map<int,std::queue<std::string>> &citynames)
{
    Faction *faction = new Faction();
    faction->id = 0;
    faction->autoPlayer = true;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->rates[0] = 1;
    faction->rates[1] = 0;
    faction->rates[2] = 0;
    faction->rates[3] = 0;
    
    factions.push_back(faction);

    faction = new Faction();
    faction->id = 1;
    faction->autoPlayer = true;
    strcpy(faction->name,"Romans");
    faction->red = 255;
    faction->green = 255;
    faction->blue = 255;
    faction->rates[0] = 1;
    faction->rates[1] = 0;
    faction->rates[2] = 0;
    faction->rates[3] = 0;
    factions.push_back(faction);


    faction = new Faction();
    faction->id = 2;
    faction->autoPlayer = true;
    strcpy(faction->name,"Greeks");
    faction->red = 0;
    faction->green = 0;
    faction->blue = 255;
    faction->rates[0] = 1;
    faction->rates[1] = 0;
    faction->rates[2] = 0;
    faction->rates[3] = 0;
    factions.push_back(faction);

    for (auto& f: factions)
    {
        std::vector<coordinate> list;
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map.set(lat,lon).code==LAND)
                {
                    list.push_back(coordinate(lat,lon));
                }
            }
    }
}

void assignProductionRates(Map &mmp, std::vector<Resource*> &resources)
{
    for(int lat=mmp.minlat;lat<mmp.maxlat;lat++)
        for (int lon=mmp.minlon;lon<mmp.maxlon;lon++)
        {
            for(auto &r:resources)
            {
                mmp.set(lat,lon).resource_production_rate.push_back(0);
            }

            if (mmp.set(lat,lon).code==OCEAN)       // Water
            {
                mmp.set(lat,lon).resource_production_rate[FOOD]     = 1;
                mmp.set(lat,lon).resource_production_rate[TRADE]    = 1;

                if (mmp.set(lat,lon).resource==FISH) mmp.set(lat,lon).resource_production_rate[FOOD] = 3;
                if (mmp.set(lat,lon).resource==OIL)  mmp.set(lat,lon).resource_production_rate[SHIELDS] = 2;
            }
            else
            if (mmp.set(lat,lon).code==LAND)       // Land
            {
                // @FIXME Adjust the basic production rate of each tile
                mmp.set(lat,lon).resource_production_rate[FOOD] = 1;

                //printf("Bioma %x\n",mmp(lat,lon).bioma);
                if (mmp.set(lat,lon).code == LAND && mmp.set(lat,lon).bioma == LANDBIOMA) // Regular land
                {
                    mmp.set(lat,lon).resource_production_rate[FOOD] = 2;
                }
                if (mmp.set(lat,lon).bioma/16==GRASSLAND/16) // Grassland
                {
                    mmp.set(lat,lon).resource_production_rate[FOOD] = 3;
                    if (mmp.set(lat,lon).resource==GEOSHIELD) mmp.set(lat,lon).resource_production_rate[SHIELDS] = 1;
                }
                if (mmp.set(lat,lon).bioma/16==RIVER/16) // River
                {
                    mmp.set(lat,lon).resource_production_rate[FOOD]  = 4;
                    mmp.set(lat,lon).resource_production_rate[TRADE] = 1;
                }
                if (mmp.set(lat,lon).bioma/16==DESERT/16) // Desert
                {
                    mmp.set(lat,lon).resource_production_rate[FOOD] = 1;
                }
                if (mmp.set(lat,lon).bioma/16==SWAMP/16) // Swamps
                {
                    mmp.set(lat,lon).resource_production_rate[FOOD] = 1;
                    mmp.set(lat,lon).resource_production_rate[TRADE] = 1;
                }
                if (mmp.set(lat,lon).bioma/16==PLAINS/16) // Plains
                {
                    mmp.set(lat,lon).resource_production_rate[FOOD] = 1;
                    mmp.set(lat,lon).resource_production_rate[SHIELDS] = 1;
                }
                if (mmp.set(lat,lon).bioma/16==HILLS/16) // Hills
                {
                    mmp.set(lat,lon).resource_production_rate[SHIELDS] = 1;
                }
                if (mmp.set(lat,lon).bioma/16==FOREST/16) // Forests
                {
                    mmp.set(lat,lon).resource_production_rate[SHIELDS] = 2;
                    if (mmp.set(lat,lon).resource==GAME) mmp.set(lat,lon).resource_production_rate[FOOD] = 2;
                    if (mmp.set(lat,lon).resource==GAME) mmp.set(lat,lon).resource_production_rate[SHIELDS] = 3;
                }
                if (mmp.set(lat,lon).bioma/16==DESERT/16)   // Deserts
                {
                    if (mmp.set(lat,lon).resource==COAL) mmp.set(lat,lon).resource_production_rate[SHIELDS] = 2;
                    if (mmp.set(lat,lon).resource==OIL)  mmp.set(lat,lon).resource_production_rate[SHIELDS] = 3;
                    if (mmp.set(lat,lon).resource==OASIS) {mmp.set(lat,lon).resource_production_rate[FOOD] = 3;
                                                            mmp.set(lat,lon).resource_production_rate[TRADE] = 1;}
                }
                if (mmp.set(lat,lon).bioma/16==MOUNTAINS/16) // Mountains
                {
                    mmp.set(lat,lon).resource_production_rate[SHIELDS] = 1;
                    if (mmp.set(lat,lon).resource==COAL) mmp.set(lat,lon).resource_production_rate[SHIELDS] = 2;
                }

                if (mmp.set(lat,lon).bioma/16==ARCTIC/16) // Mountains
                {
                    if (mmp.set(lat,lon).resource==SEAL) mmp.set(lat,lon).resource_production_rate[FOOD] = 3;
                }

                if (mmp.set(lat,lon).resource==GEMS) mmp(lat,lon).resource_production_rate[CULTURE] = 2;
                if (mmp.set(lat,lon).resource==GOLD) 
                {
                    mmp.set(lat,lon).resource_production_rate[COINS]    = 2;
                    mmp.set(lat,lon).resource_production_rate[CULTURE]  = 1;
                }
            }
        }
}


struct Coordinator 
{
    int controllingid;
    int faction;
    bool endofturn = false;
};


void update(Map &map, Factions &factions, Cities cities, Units units, Coordinator controller)
{
    for(auto& f:factions)
    {
        //printf("Faction %d - %s red %d\n",f->id,factions[f->id]->name,f->red);
        f->pop = 0;
        f->coins = 0;
    }

    // Update all the time if the city is or not defended...
    for(auto& [cid,c]:cities)
    {
        factions[c->faction]->pop += c->pop;
        c->noDefense();
        for(auto& [k, u] : units) 
        {
            if (u->latitude == c->latitude && u->longitude == c->longitude)
            {
                c->setDefense();
            }
        }

        // @FIXME: This is a workaround
        if (!c->workingOn(0,0))
        {
            map.set(c->latitude+0, c->longitude+0).setCityOwnership(c->faction, c->id);        
        }
        c->deAssigntWorkingTile();


        // @NOTE Collect taxes....
        factions[c->faction]->coins += c->resources[COINS];

        // @FIXME: Spread culture

        // @FIXME: Collect science.

    }

    // GoTo Function
    if (units.find(controller.controllingid)!=units.end() && units[controller.controllingid]->isAuto())
    {
        // Determine where to move

    }

    // Autoplayer
    if (factions[controller.faction]->autoPlayer)
    {

        //autoPlayer();

    }

    //adjustMovements();

    //processOrders();


    if (controller.endofturn)
    {
        controller.endofturn=false;
        factions[controller.faction]->done();

        // Reset all the remaining available moves for all the units that belong to the controller faction.
        for (auto& [k, u] : units) 
        {
            if (u->faction == controller.faction) u->availablemoves = 0;
        }

        if (controller.faction<factions.size()-1) 
        {
            controller.faction++;
            //setUpFaction();    
        }

    }
    
    //if (endOfTurnForAllFactions())
    //{
        // Everybody played their turn, end of year, and start it over.....
    //    endOfYear();
    //    controller.faction = 0;     // Restart the turn from the first faction.
    //    setUpFaction();
    //}  


}

// GAME Model Update
/** 
void update(Map &map, Factions &factions)
{
    for(auto& f:factions)
    {
        //printf("Faction %d - %s red %d\n",f->id,factions[f->id]->name,f->red);
        f->pop = 0;
        f->coins = 0;
    }

    // Update all the time if the city is or not defended...
    for(auto& [cid,c]:cities)
    {
        factions[c->faction]->pop += c->pop;
        c->noDefense();
        for(auto& [k, u] : units) 
        {
            if (u->latitude == c->latitude && u->longitude == c->longitude)
            {
                c->setDefense();
            }
        }

        // @FIXME: This is a workaround
        if (!c->workingOn(0,0))
        {
            map.set(c->latitude+0, c->longitude+0).setCityOwnership(c->faction, c->id);        
        }
        c->deAssigntWorkingTile();


        // @NOTE Collect taxes....
        factions[c->faction]->coins += c->resources[COINS];

        // @FIXME: Spread culture

        // @FIXME: Collect science.

    }


    // GoTo Function
    if (units.find(controller.controllingid)!=units.end() && units[controller.controllingid]->isAuto())
    {
        // First build the tree map of the available land.
        // Calculate the path to the target.

        bool ok = false;
        
        coordinate c = goTo(units[controller.controllingid],ok);
        
        if (ok)
        {
            controller.registers.roll = sgnz(c.lon-units[controller.controllingid]->longitude );
            controller.registers.pitch = sgnz(c.lat-units[controller.controllingid]->latitude );
        }
        else
        {
            // Cancel goto operation and make a sound.
            if (!units[controller.controllingid]->arrived()) blocked();
            units[controller.controllingid]->resetGoTo();
        }

        units[controller.controllingid]->arrived();

    }


    // Autoplayer
    if (factions[controller.faction]->autoPlayer)
    {

        autoPlayer();

    }

    adjustMovements();

    if (controller.endofturn)
    {
        controller.endofturn=false;
        factions[controller.faction]->done();

        // Reset all the remaining available moves for all the units that belong to the controller faction.
        for (auto& [k, u] : units) 
        {
            if (u->faction == controller.faction) u->availablemoves = 0;
        }

        if (controller.faction<factions.size()-1) 
        {
            controller.faction++;
            setUpFaction();    
        }

    }

    processCommandOrders();
    
    if (endOfTurnForAllFactions())
    {
        // Everybody played their turn, end of year, and start it over.....
        endOfYear();
        controller.faction = 0;     // Restart the turn from the first faction.
        setUpFaction();
    }

}**/

void placeThisUnit(float flat, float flon, int size, const char* filename, int red, int green, int blue)
{

}

void place(int x, int y, int sizex, int sizey, const char* modelName)
{

}

void placeWord(float x, float y, int sizex, int sizey, const char* word, int yoffset)
{

}

void placeThisTile(int lat, int lon, int size, const char* filename)
{

}

void placeThisCity(int lat, int lon, int red, int green, int blue)
{

}

Factions factions;

int main() {
    Map map;
    Tiles tiles;
    Commodities commodities;
    std::unordered_map<int,std::queue<std::string>> citynames;
    std::vector<Resource*> resources;



    map.init();

    initTiles(tiles);
    initCommodities(commodities);
    initNaming(citynames);
    initResources(resources);

    initMap(map, tiles);

    printf("Map minlat %d maxlat %d minlon %d maxlon %d\n", map.minlat, map.maxlat, map.minlon, map.maxlon);
    printf("Tile value at value 0 %s\n", tiles[0].c_str());

    assignProductionRates(map, resources);

    initFactions(factions,map, citynames);

    printf("Faction name %s\n", factions[0]->name);

    // At this point everything is set up.

    std::unordered_map<int, Unit*> units;
    std::unordered_map<int, City*> cities;

    Coordinator controller;

    // Now the logic goes like this.
    // 1. Iterate through all the factions.
    // 2. For each faction, and activate each unit.
    // 3. For each unit, decide what to do.
    
    int year = -4000;
    bool wincondition = false;
    while (!wincondition)
    {
        update(map, factions,cities, units,controller);
        year++;

        if (year == 2000)
            break;
    }


    return 0;

}
