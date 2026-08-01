#include <sys/stat.h>
#include "mapmodel.h"
#include "tiles.h"
#include "resources.h"
#include "Faction.h"
#include "City.h"
#include "units/Unit.h"
#include "units/Settler.h"
#include "units/Warrior.h"
#include "units/Worker.h"
#include "units/Horseman.h"
#include "units/Trireme.h"
#include "units/Archer.h"
#include "units/Swordman.h"
#include "units/Spearman.h"
#include "units/Axeman.h"
#include "units/Horsearcher.h"
#include "units/Galley.h"
#include "units/Scout.h"
#include "units/Warelephant.h"
#include "units/Chariot.h"
#include "units/Pretorian.h"
#include "units/Spy.h"
#include "buildings/Building.h"
#include "buildings/Palace.h"
#include "buildings/Barracks.h"
#include "buildings/Granary.h"
#include "buildings/Collosseum.h"
#include "buildings/Market.h"
#include "coordinator.h"
#include "improvements.h"
#include "diplomacy.h"
#include "ai.h"
#include "engine.h"
#include "messages.h"
#include "commandline.h"

typedef std::unordered_map<int, City*> Cities;
typedef std::unordered_map<int, Unit*> Units;

Tiles tiles;
Commodities commodities;
std::unordered_map<int,std::queue<std::string>> citynames;
std::vector<Resource*> resources;

Factions factions;
Units units;
Cities cities;
// Global, like the three above: engine.cpp (activateUnit) needs an extern Coordinator, and
// Unit.cpp (goTo) needs an extern Map -- both unreachable dead code in this stripped build,
// but their SYMBOLS still have to resolve at link time.
Map map;
Coordinator coordinator;
DiplomacyTable diplomacy;
std::unordered_map<int, Improvement*> improvements;

std::vector<Message> messages;

extern ImprovementEffort improvementeffort;
extern MovementCost movementcosts;

int year;

bool preloadmap;

bool loadgame;
char filegame[256];

bool autoEndOfTurn = true;
bool switchVisibleFaction = false;
bool mute = false;

int mapsize;

void built() {} 

void initMap()
{
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }    
}

void initFactions()
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

void initUnits()
{
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

        coordinate c(0,0);
        if (list.size()>0)
        {
            int r = getRandomInteger(0,list.size());
            c = list[r];
        }

        Settler *settler = new Settler();
        settler->longitude = c.lon;
        settler->latitude = c.lat;
        settler->id = getNextUnitId();
        settler->faction = f->id;
        settler->availablemoves = settler->getUnitMoves();

        units[settler->id] = settler;
        map.set(c.lat,c.lon).setOwnedBy(f->id);


        Warrior *warrior = new Warrior();
        warrior->longitude = c.lon;
        warrior->latitude = c.lat;
        warrior->id = getNextUnitId();
        warrior->faction = f->id;
        warrior->availablemoves = warrior->getUnitMoves();


        units[warrior->id] = warrior;
        map.set(c.lat,c.lon).setOwnedBy(f->id);

        Settler *settler2 = new Settler();
        settler2->longitude = c.lon;
        settler2->latitude = c.lat;
        settler2->id = getNextUnitId();
        settler2->faction = f->id;
        settler2->availablemoves = settler2->getUnitMoves();


        units[settler2->id] = settler2;
        map.set(c.lat,c.lon).setOwnedBy(f->id);
    }    
}

void assignProductionRates(Map &mmp, std::vector<Resource*> &resources)
{
    for(int lat=mmp.minlat;lat<mmp.maxlat;lat++)
        for (int lon=mmp.minlon;lon<mmp.maxlon;lon++)
        {
            mapcell &cell = mmp.set(lat,lon);

            for(auto &r:resources)
            {
                cell.addResourceProductionRate(0);
            }

            if (cell.code==OCEAN)       // Water
            {
                cell.setResourceProductionRate(FOOD, 1);
                cell.setResourceProductionRate(TRADE, 1);

                if (cell.resource==FISH) cell.setResourceProductionRate(FOOD, 3);
                if (cell.resource==OIL)  cell.setResourceProductionRate(SHIELDS, 2);
            }
            else
            if (cell.code==LAND)       // Land
            {
                // @FIXME Adjust the basic production rate of each tile
                cell.setResourceProductionRate(FOOD, 1);

                //printf("Bioma %x\n",mmp(lat,lon).bioma);
                if (cell.code == LAND && cell.bioma == LANDBIOMA) // Regular land
                {
                    cell.setResourceProductionRate(FOOD, 2);
                }
                if (cell.bioma/16==GRASSLAND/16) // Grassland
                {
                    cell.setResourceProductionRate(FOOD, 3);
                    if (cell.resource==GEOSHIELD) cell.setResourceProductionRate(SHIELDS, 1);
                }
                if (cell.bioma/16==RIVER/16) // River
                {
                    cell.setResourceProductionRate(FOOD, 4);
                    cell.setResourceProductionRate(TRADE, 1);
                }
                if (cell.bioma/16==DESERT/16) // Desert
                {
                    cell.setResourceProductionRate(FOOD, 1);
                }
                if (cell.bioma/16==SWAMP/16) // Swamps
                {
                    cell.setResourceProductionRate(FOOD, 1);
                    cell.setResourceProductionRate(TRADE, 1);
                }
                if (cell.bioma/16==PLAINS/16) // Plains
                {
                    cell.setResourceProductionRate(FOOD, 1);
                    cell.setResourceProductionRate(SHIELDS, 1);
                }
                if (cell.bioma/16==HILLS/16) // Hills
                {
                    cell.setResourceProductionRate(SHIELDS, 1);
                }
                if (cell.bioma/16==FOREST/16) // Forests
                {
                    cell.setResourceProductionRate(SHIELDS, 2);
                    if (cell.resource==GAME) cell.setResourceProductionRate(FOOD, 2);
                    if (cell.resource==GAME) cell.setResourceProductionRate(SHIELDS, 3);
                }
                if (cell.bioma/16==DESERT/16)   // Deserts
                {
                    if (cell.resource==COAL) cell.setResourceProductionRate(SHIELDS, 2);
                    if (cell.resource==OIL)  cell.setResourceProductionRate(SHIELDS, 3);
                    if (cell.resource==OASIS)
                    {
                        cell.setResourceProductionRate(FOOD, 3);
                        cell.setResourceProductionRate(TRADE, 1);
                    }
                }
                if (cell.bioma/16==MOUNTAINS/16) // Mountains
                {
                    cell.setResourceProductionRate(SHIELDS, 1);
                    if (cell.resource==COAL) cell.setResourceProductionRate(SHIELDS, 2);
                }

                if (cell.bioma/16==ARCTIC/16) // Mountains
                {
                    if (cell.resource==SEAL) cell.setResourceProductionRate(FOOD, 3);
                }

                if (cell.resource==GEMS) cell.setResourceProductionRate(CULTURE, 2);
                if (cell.resource==GOLD)
                {
                    cell.setResourceProductionRate(COINS, 2);
                    cell.setResourceProductionRate(CULTURE, 1);
                }
            }
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

void blocked()
{}

void war()
{}

void peace()
{}

void win()
{}

void march()
{}

void lose()
{}

inline void endOfYear()
{
    year++;
    for (auto& [k, u] : units)
    {
        // Units in movement debt (negative moves) recover one year of moves at a time
        // instead of getting the full refresh: crossing a tile that costs more than the
        // unit's moves-per-turn takes several turns.
        if (u->availablemoves < 0)
            u->availablemoves += u->getUnitMoves();
        else
            u->availablemoves = u->getUnitMoves();

        if (u->hasPendingMove() && u->availablemoves >= 0)
            completePendingMove(u);

        //checkUnitMeetings(u);

    }

    std::vector<int> todelete;
    for (auto& [k, c] : cities) 
    {
        // Pick two food items per one population and gather the rest.
        // If granary is present the amount of food that is required to increase the population is half.

        printf("City %s\t\t\thas %02d pop and %03d food\n",c->name,c->pop,c->resources[0]);
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

                    message(year, c->faction, "City %s has built %s.",c->name,building->name);
                    built();                
                }

            }
        }
        
        // Balance city population according to available resources.
        if (c->resources[FOOD]>100*c->pop) 
        {
            c->resources[FOOD] = 0;
            c->pop++;

            c->assignWorkingTile();
        } else 
        if (c->resources[FOOD]<0)
        {
            c->resources[FOOD] = 0;

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
        message(year, c->faction, "%s has been abandoned.",c->name);

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

void update(int value)
{
    cleanUnits();

    reSetCities();

    processGoTo();

    processWork(); 
    
    // Autoplayer
    if (factions[coordinator.a_f_id]->autoPlayer)
    {
        printf("Autoplayer for faction %d - %s\n", coordinator.a_f_id, factions[coordinator.a_f_id]->name);
        autoPlayerMoveUnits();
    }

    processCommandOrders();

    // @NOTE: Remove me if you want to wait until the user press the space bar to move ahead the end of turn.
    if (autoEndOfTurn && noMoreMovementsLeft(coordinator.a_f_id))
    {
        printf ("End of turn for faction %d - %s\n", coordinator.a_f_id, factions[coordinator.a_f_id]->name);
        coordinator.endofturn = true;
    }

    if (coordinator.endofturn)
    {
        coordinator.endofturn=false;
        factions[coordinator.a_f_id]->done();

        printf("Faction %d - %s has finished its turn.\n", coordinator.a_f_id, factions[coordinator.a_f_id]->name);

        if (coordinator.a_f_id<factions.size()-1) 
        {
            coordinator.a_f_id++;

            if (switchVisibleFaction)
                coordinator.v_f_id = coordinator.a_f_id;

            setUpFaction();  
            
            // Autoplayer
            if (factions[coordinator.a_f_id]->autoPlayer)
            {
                autoPlayerCities();
            }
        }

    }    

    if (endOfTurnForAllFactions())
    {
        printf ("All factions have finished their turn, end of year %d.\n", year);
        // Everybody played their turn, end of year, and start it over.....
        endOfYear();
        coordinator.a_f_id = 0;     // Restart the turn from the first faction.
        setUpFaction();

        // Autoplayer
        if (factions[coordinator.a_f_id]->autoPlayer)
        {
            autoPlayerCities();
        }
    }
}


int main(int argc, char *argv[]) {


    if (isPresentCommandLineParameter(argc,argv,"-seed"))
    {
        int seed = getDefaultedIntCommandLineParameter(argc,argv,"-seed",0);
        srand( seed );
        srand48(seed);
        setRandomSeed(seed);        // getRandomInteger uses its own <random> generator.
    }
    else
    {
        srand (time(NULL));
        srand48(time(NULL));
    }

    mapsize = getDefaultedIntCommandLineParameter(argc,argv,"-mapsize",DEFAULT_MAPSIZE);

    preloadmap = false;
    if (isPresentCommandLineParameter(argc,argv,"-loadmap"))
    {
        preloadmap = true;
    }

    loadgame = false;
    if (isPresentCommandLineParameter(argc,argv,"-loadgame"))
    {
        loadgame = true;
        strcpy(filegame, getCommandLineParameter(argc,argv,"-loadgame"));
        struct stat buffer;
        if (stat(filegame, &buffer) != 0) {
            std::cerr << "Error: The file " << filegame << " does not exist." << std::endl;
            exit(1);
        }
    }

    coordinator.a_f_id = 0;
    coordinator.a_u_id = 0;

    

    initTiles(tiles);
    initCommodities(commodities);
    initMovementCosts(movementcosts);
    initImprovementEffort(improvementeffort);
    initImprovements(improvements);
    initNaming(citynames);
    initResources(resources);

    MapDimension dimension = getMapDimension(mapsize);
    map.init(dimension.halfheight,dimension.halfwidth);    

    initMap();

    printf("Map minlat %d maxlat %d minlon %d maxlon %d\n", map.minlat, map.maxlat, map.minlon, map.maxlon);

    assignProductionRates(map, resources);

    initFactions();

    for (Faction* faction : factions)
    {
        printf("Faction name %s Autoplayer %s\n", faction->name, faction->autoPlayer ? "true" : "false");
        if (faction)
            faction->autoPlayer = true;
    }

    initDiplomacy(diplomacy, factions.size());

    initUnits();

    // At this point everything is set up.


    for (auto& unit : units)
    {
        printf("Unit %d - %s at (%d,%d) for faction %d\n", unit.second->id, unit.second->name, unit.second->latitude, unit.second->longitude, unit.second->faction);
    }

    //exit(22);



    // Now the logic goes like this.
    // 1. Iterate through all the factions.
    // 2. For each faction, and activate each unit.
    // 3. For each unit, decide what to do.
    
    year = -4000;
    bool wincondition = false;
    int ticks = 0;
    // Safety valve: nothing here ever calls Faction::done() unless a unit runs out of
    // moves (switchUnitIfNoMovesLeft), so with no starting units/cities the turn never
    // ends and year is stuck forever -- this loop would otherwise spin at full CPU,
    // printing "Year -4000" indefinitely (found the hard way: filled the disk when
    // redirected to a file).
    const int MAX_TICKS = 200000;
    while (!wincondition)
    {
        if (++ticks > MAX_TICKS)
        {
            printf("Safety stop: %d ticks elapsed with year stuck at %d -- the game loop is not progressing (no units/cities to end a turn).\n", MAX_TICKS, year);
            break;
        }

        printf("Year %d\n",year);
        //update(year,map, factions,cities, units,resources,coordinator, citynames);
        update(ticks);

        exit(22);

        if (year == 2000)
            break;
    }


    return 0;

}
