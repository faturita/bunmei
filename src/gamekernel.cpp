#include <fstream>
#include <sstream>
#include <array>

#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"
#include "coordinator.h"
#include "automation.h"

#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"

#include "messages.h"
#include "engine.h"

#include "tiles.h"
#include "improvements.h"
#include "diplomacy.h"

#include "savegame.h"
#include "dee.h"
#include "technologies.h"

#include "sounds/sounds.h"

extern Coordinator coordinator;
extern Controller controller;

extern std::unordered_map<int, std::string> tiles;
extern Map map;
std::unordered_map<int, std::vector<int>> resourcesxbioma;
extern MovementCost movementcosts;
extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern ImprovementBiomaRestrictions improvementbiomarestrictions;
extern std::unordered_map<int, int> commodityxresource;
extern std::unordered_map<int, int> prices;

extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::unordered_map<int, Improvement*> improvements;
extern DiplomacyTable diplomacy;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;

void update(int value);
//void replayupdate(int value);

extern int year;

extern int mapsize;
extern float mapzoom;

extern bool preloadmap;
extern bool loadgame;

extern char filegame[256];

extern bool autoEndOfTurn;
extern bool switchVisibleFaction;
extern int selectedFaction;
extern int numCivs;

// The tile production tables moved to tiles.cpp (initProductionRates / tileBaseProductionRates,
// declared in tiles.h) and assignProductionRates() to engine.cpp, which every build links --
// this file had one copy of the tables and simulate.cpp another, and the two had drifted.
// Each tile still keeps its own rates; they are just filled from the shared tables, here for
// a generated world and inside loadMap() for a loaded one, instead of being saved.

// saveMap()/loadMap() moved to mapio.cpp (see gamekernel.h), so they can be linked into
// the testcase and simulate builds too, which don't link gamekernel.cpp itself.


#include <set>
#include <map>
#include <queue>

// Returns a vector of vectors, each inner vector contains coordinates of a landmass
std::vector<std::vector<coordinate>> findLandmasses() {
    std::set<std::pair<int, int>> visited;
    std::vector<std::vector<coordinate>> landmasses;

    Map m = map;

    for (int lat = m.minlat; lat < m.maxlat; ++lat) {
        for (int lon = m.minlon; lon < m.maxlon; ++lon) {
            if (m(lat, lon).code == LAND && visited.count({lat, lon}) == 0) {
                std::vector<coordinate> landmass;
                std::queue<coordinate> q;
                q.push(coordinate(lat, lon));
                visited.insert({lat, lon});

                while (!q.empty()) {
                    coordinate c = q.front(); q.pop();
                    landmass.push_back(c);

                    // Check 4 neighbors (N, S, E, W)
                    int dlat[] = {-1, 1, 0, 0};
                    int dlon[] = {0, 0, 1, -1};
                    for (int d = 0; d < 4; ++d) 
                    {
                        int nlat;
                        int nlon;
                        coordinate co = m.displacement(c.lat,c.lon,dlat[d],dlon[d]);
                        nlat = co.lat;
                        nlon = co.lon;
                        if (nlat >= m.minlat && nlat < m.maxlat &&
                            nlon >= m.minlon && nlon < m.maxlon &&
                            m(nlat, nlon).code == LAND &&
                            visited.count({nlat, nlon}) == 0) {
                            q.push(coordinate(nlat, nlon));
                            visited.insert({nlat, nlon});
                        }
                    }
                }
                landmasses.push_back(landmass);
            }
        }
    }
    return landmasses;
}

// A connected ocean body at or under this many tiles counts as "landlocked" (a lake) rather
// than open sea, for the purpose of tagging it with the LAKE bioma -- see the call site below.
#define LANDLOCKED_OCEAN_MAX_SIZE 30

// Same BFS as findLandmasses() above, but over OCEAN cells instead of LAND.
std::vector<std::vector<coordinate>> findOceanBodies() {
    std::set<std::pair<int, int>> visited;
    std::vector<std::vector<coordinate>> oceanbodies;

    Map m = map;

    for (int lat = m.minlat; lat < m.maxlat; ++lat) {
        for (int lon = m.minlon; lon < m.maxlon; ++lon) {
            if (m(lat, lon).code == OCEAN && visited.count({lat, lon}) == 0) {
                std::vector<coordinate> oceanbody;
                std::queue<coordinate> q;
                q.push(coordinate(lat, lon));
                visited.insert({lat, lon});

                while (!q.empty()) {
                    coordinate c = q.front(); q.pop();
                    oceanbody.push_back(c);

                    // Check 4 neighbors (N, S, E, W)
                    int dlat[] = {-1, 1, 0, 0};
                    int dlon[] = {0, 0, 1, -1};
                    for (int d = 0; d < 4; ++d)
                    {
                        int nlat;
                        int nlon;
                        coordinate co = m.displacement(c.lat,c.lon,dlat[d],dlon[d]);
                        nlat = co.lat;
                        nlon = co.lon;
                        if (nlat >= m.minlat && nlat < m.maxlat &&
                            nlon >= m.minlon && nlon < m.maxlon &&
                            m(nlat, nlon).code == OCEAN &&
                            visited.count({nlat, nlon}) == 0) {
                            q.push(coordinate(nlat, nlon));
                            visited.insert({nlat, nlon});
                        }
                    }
                }
                oceanbodies.push_back(oceanbody);
            }
        }
    }
    return oceanbodies;
}

void initMap()
{
    initTiles(tiles);

    initProductionRates(productionrates);

    initCoreResources();

    initResources(resourcesxbioma);

    initMovementCosts(movementcosts);

    initImprovementEffort(improvementeffort);

    initImprovementResources(improvementresources);

    initImprovementBiomaRestrictions(improvementbiomarestrictions);

    initCommodities(commodityxresource);
    initPrices(prices);

    initImprovements(improvements);

    MapDimension dimension = getMapDimension(mapsize);
    map.init(dimension.halfheight,dimension.halfwidth);

    // The game always starts at the standard zoom (mapzoom 2, after the zoommapin() done at
    // initWorldModelling), independently of the mapsize.  Zooming out N times from there reaches
    // dimension.defaultzoom, where the whole map covers the screen and the camera locks centered.
    mapzoom = 1;
    printf("Map size %d: %dx%d, covers the screen at mapzoom %f\n",mapsize,dimension.halfwidth*2,dimension.halfheight*2,dimension.defaultzoom);

    //std::vector<coordinate> landmassseeds;

    // setupWorldModelling() (bunmei.cpp) calls initMap() BEFORE checking loadgame: without
    // loadgame here too, a -loadgame run would generate a fresh random world instead of
    // loadWorldModelling() ever getting a chance to read back the map that matches the save.
    // The map savegame() wrote is paired with the save file itself (filegame + ".map", see
    // savegame.cpp) rather than a single shared file, so each save keeps its own map.
    if (loadgame)
        loadMap(std::string(filegame) + ".map");
    else if (preloadmap)
        loadMap();
    else
    {
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
            {
                map.set(lat,lon) = mapcell(OCEAN);
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


        // The world generation constants below (landmass seeds, fill repetitions, biomas,
        // rivers, resources) were tuned for the size-1 map (72x48).  Scale them with the
        // map area so every mapsize generates a world with the same land density and the
        // same look as size 1 (size 2 has 4 times the area, so it gets 4 times of everything).
        int areascale = ((map.maxlat-map.minlat)*(map.maxlon-map.minlon)) /
                        ((MAPDIMENSIONS[0].halfheight*2)*(MAPDIMENSIONS[0].halfwidth*2));
        if (areascale<1) areascale = 1;

        // Pick a random number and use it to seed the land masses.
        // The NUMBER of landmasses does not scale with the map: a bigger map gets the same
        // 2..15 continents, but each seed walks areascale times longer so the continents
        // grow with the map area (a random walk of 4x the steps spans 2x the diameter,
        // exactly the linear scale of the size-2 map).
        int r=getRandomInteger(2,15);

        for(int i=0;i<r;i++)
        {
            int lat = getRandomInteger(map.minlat,map.maxlat-1);
            int lon = getRandomInteger(map.minlon,map.maxlon-1);

            for(int stretch=0;stretch<areascale;stretch++)
            while (getRandomInteger(0,100)>2)
            {
                map.set(lat,lon) = mapcell(LAND);
                int dir=getRandomInteger(0,3);
                if (dir==0) lat-=1;
                if (dir==1) lat+=1;
                if (dir==2) lon+=1;
                if (dir==3) lon-=1;
            }
            //landmassseeds.push_back(coordinate(lat,lon));
        }

        // Fill in randomly the land masses with land.
        // The fill only succeeds next to existing land, and a continent 4 times bigger has
        // only 2 times the coastline, so the attempts must scale with areascale^1.5 (not
        // just the area) to grow the continents proportionally.
        int fillreps = (int)(5000.0*areascale*sqrt((double)areascale));
        int energy = 100000*areascale;
        for(int rep=0;rep<fillreps;rep++)
        {
            int lat = getRandomInteger(map.minlat,map.maxlat-1);
            int lon = getRandomInteger(map.minlon,map.maxlon-1);

            int north,south,east,west;
            north = map(lat-1,lon).code;
            south = map(lat+1,lon).code;
            east  = map(lat,lon+1).code;
            west  = map(lat,lon-1).code;

            if (energy>0) if ((south+north+east+west)>=1)
            {
                map.set(lat,lon) = mapcell(LAND);
                energy--;
            }
        }

        // Now go through all the spots and fill in with more land to fill the gaps.
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
            {
                int north,south,east,west;
                north = map(lat-1,lon).code;
                south = map(lat+1,lon).code;
                east  = map(lat,lon+1).code;
                west  = map(lat,lon-1).code;

                if (energy>0) if ((south+north+east+west)>=3)
                {
                    map.set(lat,lon) = mapcell(LAND);
                }
            }

        // Pick the land biomas.
        for(int i=0;i<1000*areascale;i++)
        {
            int lat = getRandomInteger(map.minlat,map.maxlat-1);
            int lon = getRandomInteger(map.minlon,map.maxlon-1);

            // @FIXME: Pick this dynamically from the BIOMAS enum or something better
            int biomalist[] = {ARCTIC,DESERT,FOREST,GRASSLAND,HILLS,JUNGLE,MOUNTAINS,PLAINS,SWAMP,TUNDRA};

            int bioma = biomalist[getRandomInteger(0,9)];

            while (getRandomInteger(0,100)>2)
            {
                if (map(lat,lon).code==LAND)
                {
                    map.set(lat,lon).bioma = bioma;
                    int dir=getRandomInteger(0,3);
                    if (dir==0) lat-=1;
                    if (dir==1) lat+=1;
                    if (dir==2) lon+=1;
                    if (dir==3) lon-=1;
                }

            }
        }

        // Set the single water bioma.
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).code==OCEAN)
                {
                    map.set(lat,lon).bioma = OCEANBIOMA;
                }
            }

        // Pick the river sources.
        std::vector<coordinate> riversources;

        for(int i=0;i<100*areascale;i++)
        {
            int lat = getRandomInteger(map.minlat,map.maxlat-1);
            int lon = getRandomInteger(map.minlon,map.maxlon-1);

            if (map(lat,lon).code==LAND)
            {
                riversources.push_back(coordinate(lat,lon));
            }
        }


        // Follow the sources and paint the rivers until they reach the ocean.
        for(auto &river:riversources)
        {
            int lat = river.lat;
            int lon = river.lon;
            //printf("River upstream %d,%d\n",lat,lon) ;

            int bioma = RIVER;

            int dir=0;

            int counter = 0;

            // Rivers that start deep inside a big continent can wander for an extremely long
            // time before touching the ocean (each step retries up to 100 candidates), which
            // froze the generation on mapsize 2+.  Give the walk a budget that grows with the
            // linear size of the map; a river that runs out of budget simply ends inland.
            int rivermaxsteps = (int)(1000.0*sqrt((double)areascale));

            while (map(lat,lon).code==LAND)
            {
                if (counter++ >= rivermaxsteps) break;

                map.set(lat,lon).bioma = bioma;

                if (map.north(lat,lon).code==OCEAN)
                {
                    dir=0;
                } else if (map.south(lat,lon).code==OCEAN)
                {
                    dir=1;
                } else if (map.east(lat,lon).code==OCEAN)
                {
                    dir=2;
                } else if (map.west(lat,lon).code==OCEAN)
                {
                    dir=3;
                } else {
                    int north,south,east,west;
                    int c=100;
                    do 
                    {
                        int llat = lat; int llon = lon;
                        dir=getRandomInteger(0,3);
                        if (dir==0) llat-=1;
                        if (dir==1) llat+=1;
                        if (dir==2) llon+=1;
                        if (dir==3) llon-=1;
                        
                        // RIVER is 0xa0
                        int west = (map.west(llat,llon).bioma & 0xf0 ^ 0xa0)>0;west=west?0:1;
                        int south = (map.south(llat,llon).bioma & 0xf0 ^ 0xa0)>0;south=south?0:1;
                        int east = (map.east(llat,llon).bioma & 0xf0 ^ 0xa0)>0;east=east?0:1;
                        int north = (map.north(llat,llon).bioma & 0xf0 ^ 0xa0)>0;north=north?0:1;

                        //printf("Neighbours %d\n",north+south+east+west) ;

                        if ((north+south+east+west)<=2 || c--==0)
                        {
                            break;
                        }

                    } while (true);

                }
                //if (!keepsearching) break;
                if (dir==0) lat-=1;
                if (dir==1) lat+=1;
                if (dir==2) lon+=1;
                if (dir==3) lon-=1;
                //if (counter++==4) break;

                if (map(lat,lon).code==OCEAN) 
                {
                    map.set(lat,lon).bioma = RIVER;
                }
            }
        }

        // For all the land, set the default bioma to LANDBIOMA (0x01) which is the green base land.
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).code==LAND && map(lat,lon).bioma==0)       // Skip all the first biomas which are river mouths
                {
                    map.set(lat,lon).bioma = LANDBIOMA;
                }
            }
            
        // @NOTE: ---- At this point all the BIOMAS from all the cells have been decided. @FIXME add asserts to check this.

        // Now, pick several places where to put special resources.
        std::vector<coordinate> resourcelocations;

        for(int i=0;i<300*areascale;i++)
        {
            int lat = getRandomInteger(map.minlat,map.maxlat-1);
            int lon = getRandomInteger(map.minlon,map.maxlon-1);

            if (map(lat,lon).code==LAND)
            {
                resourcelocations.push_back(coordinate(lat,lon));
                
            }
        }

        // Pick water spots where to put some resources.
        for(int i=0;i<50*areascale;i++)
        {
            int lat = getRandomInteger(map.minlat,map.maxlat-1);
            int lon = getRandomInteger(map.minlon,map.maxlon-1);

            if (map(lat,lon).code==OCEAN)
            {
                resourcelocations.push_back(coordinate(lat,lon));
                
            }
        }

        // Now go through all the resource locations and pick a resource for each one.
        for(auto &resource:resourcelocations)
            {
                int lat = resource.lat;
                int lon = resource.lon;
                printf("Resources Location Code %d  Bioma %d @ %d,%d\n",map(lat,lon).code, map(lat,lon).bioma, lat,lon) ;

                std::vector<int> available_resources = resourcesxbioma[map(lat,lon).bioma];

                if (available_resources.size()>0)
                {
                    int res = available_resources[getRandomInteger(0,available_resources.size()-1)];

                    map.set(lat,lon).resource = res;
                }
            }


        //map.set(-8,-4).code = 0;
        //map.set(-8,-4).bioma = 0xa0;

        //map.set(4,4).resource = 0x106;



        // Takes all the biomas and calculate the bioma values according to their neighbours.
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).code==LAND && map(lat,lon).bioma>5)       // Skip all the first biomas which are river mouths
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


        // Adjust the estuaries (oceans with rivers that flow into them)
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).code==OCEAN && map(lat,lon).bioma==RIVER)
                {
                    int biom = 0xa0;  // 0xa0 is RIVER
                    int b1 = (map.west(lat,lon).bioma & 0xf0 ^ biom)>0;b1=b1?0:1;
                    int b2 = (map.south(lat,lon).bioma & 0xf0 ^ biom)>0;b2=b2?0:1;
                    int b3 = (map.east(lat,lon).bioma & 0xf0 ^ biom)>0;b3=b3?0:1;
                    int b4 = (map.north(lat,lon).bioma & 0xf0 ^ biom)>0;b4=b4?0:1;

                    //printf(" %x %x %x %x: %x\n",b4,b3,b2,b1, b4<<3 | b3<<2 | b2<<1 | b1);

                    int val = (b4<<3 | b3<<2 | b2<<1 | b1);

                    if (b1) map.set(lat,lon).bioma = RIVER_MOUTH_W;
                    if (b2) map.set(lat,lon).bioma = RIVER_MOUTH_S;
                    if (b3) map.set(lat,lon).bioma = RIVER_MOUTH_E;
                    if (b4) map.set(lat,lon).bioma = RIVER_MOUTH_N;

                }
            }
    }

    // Summary of the world: how much of it is land.
    {
        int landcount = 0;
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
                if (map(lat,lon).code==LAND) landcount++;
        int totalcells = (map.maxlat-map.minlat)*(map.maxlon-map.minlon);
        printf("Generated world: %d land cells of %d (%d%%)\n",landcount,totalcells,landcount*100/totalcells);
        fflush(stdout);
    }
 
    std::vector<std::vector<coordinate>> landmassseeds = findLandmasses();

    printf("Detected %d landmasses\n",landmassseeds.size()) ;

    for(auto &llm: landmassseeds)
    {
        coordinate lm = llm[0];
        printf("Landmass Seed at %d,%d\n",lm.lat,lm.lon) ;
        coordinate c = lm;
        int land = determineLandMass(c);
        printf(" Landmass Size %d\n",land) ;
    }
    fflush(stdout);

    // Landlocked oceans (small enclosed ocean bodies, which map generation can produce by
    // chance) become the LAKE bioma so Irrigation can use them as a water source (see
    // tileHasWaterOasisOrIrrigationNearby, engine.cpp). Cells already carrying a river/estuary bioma
    // are left alone (still a valid water source for irrigation either way).
    std::vector<std::vector<coordinate>> oceanbodies = findOceanBodies();
    int lakecount = 0;
    int lakecells = 0;
    for (auto &body : oceanbodies)
    {
        if ((int)body.size() <= LANDLOCKED_OCEAN_MAX_SIZE)
        {
            // The tagging rule itself lives in engine.cpp so it can be tested (gamekernel.cpp
            // is not linked into the testcase build); this loop only decides WHICH bodies are
            // small enough to count as lakes.
            lakecells += tagLakeCells(body);
            lakecount++;
        }
    }
    // Cells, not just bodies: the old message counted qualifying bodies whether or not it
    // changed anything, which is exactly why the bug above stayed invisible.
    printf("Detected %d lake(s) from landlocked oceans (%d tiles tagged)\n", lakecount, lakecells);
    fflush(stdout);

    int lat = 23;
    int lon = 35;

    // Fresh world: give every tile its rates. A LOADED map gets them inside loadMap() (the
    // branch above), so both paths arrive here with the same per-tile state -- re-running it
    // is harmless either way.
    assignProductionRates(map);
}



// Static definition of the game's civilizations.  id must match the row's position (it is
// also the index used into the diplomacy table).
struct FactionDefinition
{
    int id;
    const char* name;
    int red, green, blue;
    float rates[FUNDAMENTAL_RATES];
    bool autoPlayer;
    void (*song)();
};

static const FactionDefinition FACTION_DEFINITIONS[] = {
    { 0,  "Vikings",     255, 0,   0,   {0.5, 0, 0, 0.5}, true, vikings     },
    { 1,  "Romans",      255, 255, 255, {0.5, 0.5, 0, 0}, true, romans      },
    { 2,  "Greeks",      0,   0,   255, {0.5, 0.5, 0, 0}, true, greeks      },
    { 3,  "Chinese",     0,   255, 255, {0.5, 0.5, 0, 0}, true, chinese     },
    { 4,  "Egyptians",   255, 255, 0,   {0.5, 0.5, 0, 0}, true, egyptians   },
    { 5,  "Babylonians", 0,   255, 0,   {0.5, 0.5, 0, 0}, true, babylonians },
    { 6,  "English",     100, 33,  100, {0.5, 0.5, 0, 0}, true, english     },
    { 7,  "Mongols",     128, 128, 128, {0.5, 0.5, 0, 0}, true, mongols     },
    { 8,  "Russians",    160, 20,  40,  {0.5, 0.5, 0, 0}, true, russians    },
    { 9,  "Zulus",       20,  130, 40,  {0.5, 0.5, 0, 0}, true, zulus       },
    { 10, "Germans",     40,  70,  130, {0.5, 0.5, 0, 0}, true, germans     },
    { 11, "French",      90,  140, 230, {0.5, 0.5, 0, 0}, true, french      },
    { 12, "Aztec",       235, 175, 30,  {0.5, 0.5, 0, 0}, true, aztec       },
    { 13, "Americans",   40,  170, 160, {0.5, 0.5, 0, 0}, true, americans   },
    { 14, "Indians",     190, 190, 190, {0.5, 0.5, 0, 0}, true, indians     },
    { 15, "Incan",       200, 130, 20,  {0.5, 0.5, 0, 0}, true, incan       },
    { 16, "Japanese",    245, 225, 230, {0.5, 0.5, 0, 0}, true, japanese    },
    { 17, "Spanish",     200, 50,  20,  {0.5, 0.5, 0, 0}, true, spanish     },
};

#define NUMBER_OF_FACTION_DEFINITIONS ((int)(sizeof(FACTION_DEFINITIONS)/sizeof(FACTION_DEFINITIONS[0])))

// -civs N (bunmei.cpp numCivs): how many of the defined civilizations actually get loaded.
#define MIN_CIVS 2
#define MAX_CIVS NUMBER_OF_FACTION_DEFINITIONS

void initFactions()
{
    // numCivs<0 (unset, the default) means "no cap": load every defined civilization, same
    // as before -civs existed. An explicit value is clamped into [MIN_CIVS, MAX_CIVS] --
    // MAX_CIVS tracks NUMBER_OF_FACTION_DEFINITIONS so "-civs 8" and omitting -civs load the
    // same full set.
    int civsToLoad = NUMBER_OF_FACTION_DEFINITIONS;
    if (numCivs >= 0)
    {
        civsToLoad = numCivs;
        if (civsToLoad < MIN_CIVS)
        {
            printf("-civs %d is below the minimum of %d; using %d.\n", numCivs, MIN_CIVS, MIN_CIVS);
            civsToLoad = MIN_CIVS;
        }
        else if (civsToLoad > MAX_CIVS)
        {
            printf("-civs %d is above the maximum of %d; using %d.\n", numCivs, MAX_CIVS, MAX_CIVS);
            civsToLoad = MAX_CIVS;
        }
    }

    int civIndex = 0;
    for (auto &def : FACTION_DEFINITIONS)
    {
        if (civIndex++ >= civsToLoad)
            break;
        Faction *faction = new Faction();
        faction->id = def.id;
        strcpy(faction->name, def.name);
        faction->red = def.red;
        faction->green = def.green;
        faction->blue = def.blue;
        for (int i = 0; i < FUNDAMENTAL_RATES; i++)
            faction->rates[i] = def.rates[i];
        faction->autoPlayer = def.autoPlayer;
        printf ("selected faaction: %d, def.id: %d, autoPlayer: %d\n", selectedFaction, def.id, (selectedFaction != def.id));
        if (selectedFaction >=0)
            faction->autoPlayer = (selectedFaction != def.id);

        faction->song = def.song;

        factions.push_back(faction);
    }


    initNaming(citynames);

}

void worldStep(int value)
{
    update(value);
}

void initUnits()
{
    // One distinct random LAND tile per faction -- no two civilizations start on the same
    // tile (see ai.cpp:pickFactionStartTiles). simulate.cpp's initUnits() mirrors this.
    std::vector<coordinate> starts = pickFactionStartTiles((int)factions.size());

    int fi = 0;
    for (auto& f: factions)
    {
        coordinate c = fi < (int)starts.size() ? starts[fi] : coordinate(0,0);
        fi++;

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


void initWorldModelling()
{
    year = -4000;

    initFactions();
    initDiplomacy(diplomacy, factions.size());

    // One tech graph per faction, everybody starting at the root (Language, registered in
    // the DEE too). initTechnologies() lives in technologies.cpp so the simulator -- which
    // does not link gamekernel.cpp -- sets this up exactly the same way.
    initTechnologies(techtree, factions.size(), dee);


    initUnits();

    // @NOTE: Turn order (a_f_id) must always start at the first faction so the round-robin
    // in update() (bunmei.cpp) walks through every faction each turn. Only the visible
    // faction (v_f_id, whose map/vision the human sees) should follow -faction.
    coordinator.a_f_id = factions[0]->id;

    if (selectedFaction >= 0)
    {
        coordinator.v_f_id = factions[selectedFaction]->id;
    }
    else
    {
        coordinator.v_f_id = factions[0]->id;
    }

    coordinator.a_u_id = nextUnitId(coordinator.a_f_id);
    //factions[0]->autoPlayer = true;

    for (auto& f: factions)
    {
        // Welcome message for all the factions.
        message(year, f->id, "Sir, our destiny is to build a great empire.  We must start by building our first city.");

        if (!f->autoPlayer && f->song) f->song();

    }

    // @NOTE: Allow to finish the turn automatically when all units have moved, so the player does not have to click "End Turn" every time.
    autoEndOfTurn = true;
    switchVisibleFaction = false;


    centermapinmap(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);
    zoommapin();     
}

void loadWorldModelling()
{
    printf("Loading saved game from %s...\n", filegame);

    // Verify BEFORE anything is applied: magic, payload length, MD5, and a format version
    // this build speaks. A savegame that fails any of those is not loaded at all -- half a
    // world restored from a corrupt file is worse than refusing, and there is nothing
    // sensible to fall back to this deep into setup (the caller has already generated or
    // loaded the map for this save).
    std::string savedata;
    SaveGameInfo saveinfo;
    if (!readSaveGame(filegame, savedata, saveinfo))
    {
        printf("Refusing to load %s -- see above. Aborting rather than starting a half-restored game.\n", filegame);
        exit(1);
    }

    std::istringstream in(savedata, std::ios::binary);

    // The map itself was already restored by initMap() (setupWorldModelling calls initMap()
    // before this function; initMap() loads saved_map.dat instead of generating a fresh world
    // whenever loadgame is set, same as it does for preloadmap).

    // Load year, this is the first thing saved in the savegame file.
    in.read(reinterpret_cast<char*>(&year), sizeof(year));

    initFactions();
    initDiplomacy(diplomacy, factions.size());

    // One tech graph per faction, everybody starting at the root (Language, registered in
    // the DEE too). initTechnologies() lives in technologies.cpp so the simulator -- which
    // does not link gamekernel.cpp -- sets this up exactly the same way.
    initTechnologies(techtree, factions.size(), dee);


    loadCities(in);

    loadUnits(in);

    // The DEE registry and each faction's tech progress, in the order savegame() wrote them.
    // After loadCities/loadUnits deliberately: loadDependencies() REPLACES the registry, so it
    // must run once nothing else is going to register anything.
    loadDependencies(in);
    loadTechnologies(in);

    // Last: unit status and cargo name the units they belong to, so every unit must exist.
    loadUnitStatus(in);

    //initUnits();

    coordinator.a_f_id = factions[0]->id;

    if (selectedFaction >= 0)
    {
        coordinator.v_f_id = factions[selectedFaction]->id;
    }
    else
    {
        coordinator.v_f_id = factions[0]->id;
    }

    coordinator.a_u_id = nextUnitId(coordinator.a_f_id);
    //factions[0]->autoPlayer = false;

    // @NOTE: Allow to finish the turn automatically when all units have moved, so the player
    // does not have to click "End Turn" every time -- same as initWorldModelling() (new game);
    // this was missing here, so a loaded game never auto-ended a turn once units ran out of
    // moves (autoEndOfTurn defaults to false, only initWorldModelling() ever set it to true).
    autoEndOfTurn = true;
    switchVisibleFaction = false;

    centermapinmap(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);
    zoommapin();
}