#include "mapmodel.h"
#include "tiles.h"
#include "resources.h"
#include "Faction.h"
#include "City.h"
#include "units/Unit.h"
#include "coordinator.h"
#include "engine.h"

typedef std::unordered_map<int, City*> Cities;
typedef std::unordered_map<int, Unit*> Units;

Factions factions;
Units units;
Cities cities;

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

inline bool endOfTurnForAllFactions(Factions &factions)
{
    for(auto& f:factions)
    {
        if (!f->isDone())
            return false;
    }
    return true;
}

void setUpFaction(Map &map, Factions &factions, Cities &cities, Units &units, Coordinator &coordinator)
{
    coordinator.reset();
    coordinator.a_u_id=nextMovableUnitId(coordinator.a_f_id);
    if (units.find(coordinator.a_u_id)!=units.end())
    {
        map.setCenter(0,factions[coordinator.a_f_id]->mapoffset);
        coordinate c(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);  
    }      
}

inline void endOfYear(int &year, Map &map, Factions &factions, Cities &cities, Units &units, std::vector<Resource*> &resources, Coordinator &coordinator)
{
    year++;
    for (auto& [k, u] : units) 
    {
        u->availablemoves = u->getUnitMoves();
    }

    std::vector<int> todelete;
    for (auto& [k, c] : cities) 
    {
        // Pick two food items per one population and gather the rest.
        // If granary is present the amount of food that is required to increase the population is half.

        printf("City %s\t\thas %02d pop and %03d food\n",c->name,c->pop,c->resources[0]);
        // Go through all the map locations and gather all the resources.
        for(auto &r:resources)
        {
            c->resources[r->id] += c->getProductionRate(r->id);
        }

        // Reduce the number of resources according to what is required now.
        for(auto &r:resources)
        {
            c->resources[r->id] -= c->getConsumptionRate(r->id);
        }

        // Convert trade accordingly.  Trade is not accummulated

        c->resources[COINS] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[0]);
        c->resources[SCIENCE] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[1]);
        //c->resources[LUXURY] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[0])
        c->resources[CULTURE] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[2]);

        c->resources[TRADE]=0;
        

        // Peek the production queue.
        if (c->productionQueue.size()>0)
        {
            BuildableFactory *bf = c->productionQueue.front();
            if (c->resources[1]>=bf->cost(1))
            {
                c->resources[1] -= bf->cost(1);          // @FIXME This can be extended to more resources.

                // Access the production queue from the city, build the latest thing in the queue and move forward with the next one
                c->productionQueue.pop();
                Buildable *b = bf->create();

                if (b->getType() == BuildableType::UNIT)
                {
                    Unit *unit = (Unit*)b;
                    unit->longitude = c->longitude;
                    unit->latitude = c->latitude;
                    unit->id = getNextUnitId();
                    unit->faction = c->faction;
                    unit->availablemoves = unit->getUnitMoves();

                    units[unit->id] = unit;  
                }
                else
                {
                    Building *building = (Building*)b;
                    building->faction = c->faction;
                    c->buildings.push_back(building);

                    //message(year, c->faction, "City %s has built %s.",c->name,building->name);
                    //built();                
                }

            }
        }
        
        // Balance city population according to available resources.
        if (c->resources[0]>100)
        {
            c->resources[0] = 0;
            c->pop++;

            c->assignWorkingTile();
        } else 
        if (c->resources[0]<0)
        {
            c->resources[0] = 0;

            if (c->pop>1)
            {
                c->pop--;
                c->deAssigntWorkingTile();
            } else if (c->pop == 1)
            {
                // The city is abandoned.
                todelete.push_back(c->id);

                // @FIXME: When a city is captured with pop 1 it should be burned.
            }

        }

    }

    for(auto& cid:todelete)
    {
        City* c = cities[cid];
        // The city is abandoned.
        //message(year, c->faction, "%s has been abandoned.",c->name);

        c->pop = 0;
        c->deAssigntWorkingTile();
        map.set(c->latitude, c->longitude).releaseCityOwnership();  // The removing of the 0,0 tile.
        cities.erase(c->id);
        delete c;

        // @FIXME: Check the consistency of the map regarding that no deleted city should be still marked there
    }


    for(auto& f:factions)
    {
        f->ready();
    }
}

void switchUnitIfNoMovesLeft(Coordinator &coordinator)
{
    if (units.find(coordinator.a_u_id)!=units.end())
        if (units[coordinator.a_u_id]->availablemoves==0)
        {
            int cid = nextMovableUnitId(coordinator.a_f_id);
            if (cid != 0)
            {
                coordinator.a_u_id = cid;
            }
            else
            {
                coordinator.endofturn = true;
            }
        }
}

void update(int &year, Map &map, Factions &factions, Cities &cities, Units &units, std::vector<Resource*> &resources, Coordinator &coordinator)
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
    if (units.find(coordinator.a_u_id)!=units.end() && units[coordinator.a_u_id]->isAuto())
    {
        // Determine where to move

    }

    // Autoplayer
    if (factions[coordinator.a_f_id]->autoPlayer)
    {

        //autoPlayer();
        if (units.find(coordinator.a_u_id)!=units.end())
        {
            units[coordinator.a_u_id]->availablemoves = 0;
        }

    }

    //adjustMovements();
    switchUnitIfNoMovesLeft(coordinator);

    //processOrders();


    if (coordinator.endofturn)
    {
        coordinator.endofturn=false;
        factions[coordinator.a_f_id]->done();

        // Reset all the remaining available moves for all the units that belong to the controller faction.
        for (auto& [k, u] : units) 
        {
            if (u->faction == coordinator.a_f_id) u->availablemoves = 0;
        }

        if (coordinator.a_f_id<factions.size()-1) 
        {
            coordinator.a_f_id++;
            setUpFaction(map, factions, cities,units, coordinator);
        }

    }
    
    if (endOfTurnForAllFactions(factions))
    {
        // Everybody played their turn, end of year, and start it over.....
       endOfYear(year,map, factions, cities,units,resources, coordinator);
       coordinator.a_f_id = 0;     // Restart the turn from the first faction.
       setUpFaction(map, factions, cities, units,coordinator);
    }  


}


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


int main() {
    Map map;
    Tiles tiles;
    Commodities commodities;
    std::unordered_map<int,std::queue<std::string>> citynames;
    std::vector<Resource*> resources;

    Coordinator coordinator;

    coordinator.a_f_id = 0;
    coordinator.a_u_id = 0;

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



    // Now the logic goes like this.
    // 1. Iterate through all the factions.
    // 2. For each faction, and activate each unit.
    // 3. For each unit, decide what to do.
    
    int year = -4000;
    bool wincondition = false;
    while (!wincondition)
    {
        printf("Year %d\n",year);
        update(year,map, factions,cities, units,resources,coordinator);

        if (year == 2000)
            break;
    }


    return 0;

}
