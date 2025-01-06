#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"

#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"

#include "messages.h"
#include "engine.h"

extern Controller controller;

extern std::unordered_map<int, std::string> tiles;
extern Map map;
std::unordered_map<int, std::vector<int>> resourcesxbioma;

extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;

void update(int value);
//void replayupdate(int value);

extern int year;

void initMap()
{
    tiles[OCEAN] = "assets/assets/terrain/ocean.png";
    tiles[LAND] = "assets/assets/terrain/land.png";

    tiles[RIVER_MOUTH_W] = "assets/assets/terrain/river_mouth_w.png";
    tiles[RIVER_MOUTH_S] = "assets/assets/terrain/river_mouth_s.png";
    tiles[RIVER_MOUTH_E] = "assets/assets/terrain/river_mouth_e.png";
    tiles[RIVER_MOUTH_N] = "assets/assets/terrain/river_mouth_n.png";

    tiles[ARCTIC] = "assets/assets/terrain/arctic.png";
    tiles[DESERT] = "assets/assets/terrain/desert.png";
    tiles[FOREST] = "assets/assets/terrain/forest.png";
    tiles[GRASSLAND] = "assets/assets/terrain/grassland.png";
    tiles[HILLS] = "assets/assets/terrain/hills.png";
    tiles[JUNGLE] = "assets/assets/terrain/jungle.png";
    tiles[MOUNTAINS] = "assets/assets/terrain/mountains.png";
    tiles[PLAINS] = "assets/assets/terrain/plains.png";
    tiles[RIVER] = "assets/assets/terrain/river.png";
    tiles[SWAMP] = "assets/assets/terrain/swamp.png";
    tiles[TUNDRA] = "assets/assets/terrain/tundra.png";
    tiles[OCEANBIOMA] = "assets/assets/terrain/ocean.png";

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

    tiles[0xd0] = "assets/assets/terrain/ocean.png";

    tiles[MARBLE] = "assets/assets/terrain/marble.png"; 
    tiles[COAL] = "assets/assets/terrain/coal.png"; 
    tiles[IRON] = "assets/assets/terrain/iron.png";
    tiles[COPPER] = "assets/assets/terrain/copper.png";
    tiles[GOLD] = "assets/assets/terrain/gold.png";
    tiles[DOE] = "assets/assets/terrain/doe.png";
    tiles[FISH] = "assets/assets/terrain/fish.png";
    tiles[GAME] = "assets/assets/terrain/game.png";
    tiles[GEMS] = "assets/assets/terrain/gems.png";
    tiles[HORSE] = "assets/assets/terrain/horse.png";
    tiles[OASIS] = "assets/assets/terrain/oasis.png";
    tiles[OIL] = "assets/assets/terrain/oil.png";
    tiles[SEAL] = "assets/assets/terrain/seal.png";
    tiles[GEOSHIELD] = "assets/assets/terrain/shield.png";

    resourcesxbioma[ARCTIC]     = {MARBLE,GEMS,OIL,SEAL};
    resourcesxbioma[DESERT]     = {OASIS,OIL};
    resourcesxbioma[FOREST]     = {GAME};
    resourcesxbioma[GRASSLAND]  = {MARBLE,COAL,IRON,COPPER,GOLD,DOE,GAME,HORSE,OIL,GEOSHIELD};
    resourcesxbioma[HILLS]      = {MARBLE,COAL,IRON,COPPER,GOLD,GEMS,OIL,GEOSHIELD};
    resourcesxbioma[JUNGLE]     = {IRON,GOLD,GEMS,OIL};
    resourcesxbioma[MOUNTAINS]  = {MARBLE,COAL,IRON,COPPER,GOLD,GEMS,OIL,GEOSHIELD};
    resourcesxbioma[PLAINS]     = {DOE,GAME,HORSE,OIL,GEOSHIELD};
    resourcesxbioma[RIVER]      = {FISH};
    resourcesxbioma[SWAMP]      = {GOLD,GEMS,OIL};
    resourcesxbioma[TUNDRA]     = {MARBLE,COPPER,GOLD,GEMS,OIL,SEAL};
    resourcesxbioma[OCEANBIOMA] = {FISH,OIL};


    map.init();

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


    // Pick a random number and use it to seed the land masses.
    int r=getRandomInteger(2,15);

    for(int i=0;i<r;i++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        while (getRandomInteger(0,100)>2)
        {
            map.set(lat,lon) = mapcell(LAND);
            int dir=getRandomInteger(0,3);
            if (dir==0) lat-=1;
            if (dir==1) lat+=1;
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
    for(int i=0;i<100;i++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

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

    for(int i=0;i<100;i++)
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

        while (map(lat,lon).code==LAND)
        {
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

    for(int i=0;i<300;i++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        if (map(lat,lon).code==LAND)
        {
            resourcelocations.push_back(coordinate(lat,lon));
            
        }
    }

    // Pick water spots where to put some resources.
    for(int i=0;i<50;i++)
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

void initResources()
{
    resources.push_back(new Resource(FOOD,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(SHIELDS,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(TRADE,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(COINS,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(SCIENCE,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(CULTURE,0,"assets/assets/city/culture.png","Culture"));

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            for(auto &r:resources)
            {
                map.set(lat,lon).resource_production_rate.push_back(0);
            }

            if (map.set(lat,lon).code==OCEAN)       // Water
            {
                map.set(lat,lon).resource_production_rate[FOOD]     = 1;
                map.set(lat,lon).resource_production_rate[TRADE]    = 1;

                if (map.set(lat,lon).resource==FISH) map.set(lat,lon).resource_production_rate[FOOD] = 3;
                if (map.set(lat,lon).resource==OIL)  map.set(lat,lon).resource_production_rate[SHIELDS] = 2;
            }
            else
            if (map.set(lat,lon).code==LAND)       // Land
            {
                // @FIXME Adjust the basic production rate of each tile
                map.set(lat,lon).resource_production_rate[FOOD] = 1;

                //printf("Bioma %x\n",map(lat,lon).bioma);
                if (map.set(lat,lon).code == LAND && map.set(lat,lon).bioma == LANDBIOMA) // Regular land
                {
                    map.set(lat,lon).resource_production_rate[FOOD] = 2;
                }
                if (map.set(lat,lon).bioma/16==GRASSLAND/16) // Grassland
                {
                    map.set(lat,lon).resource_production_rate[FOOD] = 3;
                    if (map.set(lat,lon).resource==GEOSHIELD) map.set(lat,lon).resource_production_rate[SHIELDS] = 1;
                }
                if (map.set(lat,lon).bioma/16==RIVER/16) // River
                {
                    map.set(lat,lon).resource_production_rate[FOOD]  = 4;
                    map.set(lat,lon).resource_production_rate[TRADE] = 1;
                }
                if (map.set(lat,lon).bioma/16==DESERT/16) // Desert
                {
                    map.set(lat,lon).resource_production_rate[FOOD] = 1;
                }
                if (map.set(lat,lon).bioma/16==HILLS/16) // Hills
                {
                    map.set(lat,lon).resource_production_rate[SHIELDS] = 1;
                }
                if (map.set(lat,lon).bioma/16==FOREST/16) // Forests
                {
                    map.set(lat,lon).resource_production_rate[SHIELDS] = 2;
                }
                if (map.set(lat,lon).bioma/16==DESERT/16)   // Deserts
                {
                    if (map.set(lat,lon).resource==COAL) map.set(lat,lon).resource_production_rate[SHIELDS] = 2;
                    if (map.set(lat,lon).resource==OIL)  map.set(lat,lon).resource_production_rate[SHIELDS] = 3;
                    if (map.set(lat,lon).resource==OASIS) {map.set(lat,lon).resource_production_rate[FOOD] = 3;
                                                            map.set(lat,lon).resource_production_rate[TRADE] = 1;}
                }
                if (map.set(lat,lon).bioma/16==MOUNTAINS/16) // Mountains
                {
                    map.set(lat,lon).resource_production_rate[SHIELDS] = 1;
                    if (map.set(lat,lon).resource==COAL) map.set(lat,lon).resource_production_rate[SHIELDS] = 2;
                }

                if (map.set(lat,lon).resource==GEMS) map(lat,lon).resource_production_rate[CULTURE] = 2;
                if (map.set(lat,lon).resource==GOLD) 
                {
                    map.set(lat,lon).resource_production_rate[COINS]    = 2;
                    map.set(lat,lon).resource_production_rate[CULTURE]  = 1;
                }
            }
        }
}

void initFactions()
{

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    
    factions.push_back(faction);

    faction = new Faction();
    faction->id = 1;
    strcpy(faction->name,"Romans");
    faction->red = 255;
    faction->green = 255;
    faction->blue = 255;
    factions.push_back(faction);


    faction = new Faction();
    faction->id = 2;
    strcpy(faction->name,"Greeks");
    faction->red = 0;
    faction->green = 0;
    faction->blue = 255;
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


    citynames[0] = std::queue<std::string>();       // Vikings
    citynames[1] = std::queue<std::string>();       // Romans
    citynames[2] = std::queue<std::string>();       // Greeks

    citynames[0].push("Kattegate");
    citynames[0].push("Jorvik");
    citynames[0].push("Hedeby");
    citynames[0].push("Trondheim");
    citynames[0].push("Bergen");
    citynames[0].push("Stavanger");
    citynames[0].push("Kristiansand");
    citynames[0].push("Oslo");
    citynames[0].push("Stockholm");
    citynames[0].push("Copenhagen");
    citynames[0].push("Helsinki");
    citynames[0].push("Reykjavik");

    citynames[1].push("Roma");
    citynames[1].push("Caesarea");
    citynames[1].push("Carthage");
    citynames[1].push("Nicopolis");
    citynames[1].push("Byzantium");
    citynames[1].push("Brundisium");
    citynames[1].push("Camulodunum");
    citynames[1].push("Syracuse");
    citynames[1].push("Antioch");
    citynames[1].push("Palmyra");
    citynames[1].push("Cyrene");
    citynames[1].push("Alexandria");
    citynames[1].push("Gordion");
    citynames[1].push("Jerusalem");
    citynames[1].push("Ravenna");
    citynames[1].push("Artaxata");

    citynames[2].push("Atenas");
    citynames[2].push("Sparta");
    citynames[2].push("Corinto");
    citynames[2].push("Tevas");
    citynames[2].push("Delfos");
    citynames[2].push("Olimpia");
    citynames[2].push("Micenas");
    citynames[2].push("Tebas");
    citynames[2].push("Argos");
    citynames[2].push("Mileto");
    citynames[2].push("Efeso");
    citynames[2].push("Samos");
    citynames[2].push("Rodas");

}

void worldStep(int value)
{
    update(value);
}



void initWorldModelling()
{
    initFactions();   

    controller.faction = factions[0]->id;
    controller.controllingid = nextUnitId(controller.faction);
    factions[0]->autoPlayer = false;
    
    year = -4000;

    message(year, controller.faction, "Sir, our destiny is to build a great empire.  We must start by building our first city.");

    centermapinmap(units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
    zoommapin();     
}