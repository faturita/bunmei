#include <fstream>

#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"
#include "coordinator.h"

#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"

#include "messages.h"
#include "engine.h"

#include "tiles.h"

extern Coordinator coordinator;
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

extern bool preloadmap;

void assignProductionRates(Map &mmp, std::vector<Resource*> &resources)
{
    // @FIXME Encode all of this....
    initResources(resources);

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

                    if (mmp.set(lat,lon).resource==GEOSHIELD)   mmp.set(lat,lon).resource_production_rate[SHIELDS] = 2;
                    if (mmp.set(lat,lon).resource==HORSE)       mmp.set(lat,lon).resource_production_rate[SHIELDS] = 2;

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

                if (mmp.set(lat,lon).bioma/16==ARCTIC/16) // Arctic
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


void saveMap()
{
    std::ofstream out("saved_map.dat", std::ios::binary);
    if (!out) {
        printf("Error opening file for writing.\n");
        return;
    }

    for (int lat = map.minlat; lat < map.maxlat; lat++) {
        for (int lon = map.minlon; lon < map.maxlon; lon++) {
            auto &cell = map(lat, lon);
            out.write(reinterpret_cast<const char*>(&lat), sizeof(lat));
            out.write(reinterpret_cast<const char*>(&lon), sizeof(lon));
            out.write(reinterpret_cast<const char*>(&cell.code), sizeof(cell.code));
            out.write(reinterpret_cast<const char*>(&cell.bioma), sizeof(cell.bioma));
            out.write(reinterpret_cast<const char*>(&cell.resource), sizeof(cell.resource));
            // Save resource_production_rate vector size and data
            size_t sz = cell.resource_production_rate.size();
            out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            if (sz > 0)
                out.write(reinterpret_cast<const char*>(cell.resource_production_rate.data()), sz * sizeof(int));
            // Save owner if present
            out.write(reinterpret_cast<const char*>(&cell.c_id_owner), sizeof(cell.c_id_owner));
            out.write(reinterpret_cast<const char*>(&cell.f_id_owner), sizeof(cell.f_id_owner));
            out.write(reinterpret_cast<const char*>(&cell.owners), sizeof(cell.owners));
        }
    }
    out.close();
    printf("Map saved to saved_map.dat\n");
}

void loadMap()
{
    std::ifstream in("saved_map.dat", std::ios::binary);
    if (!in) {
        printf("Error opening file for reading.\n");
        return;
    }

    for (int lat = map.minlat; lat < map.maxlat; lat++) {
        for (int lon = map.minlon; lon < map.maxlon; lon++) {
            int file_lat, file_lon;
            in.read(reinterpret_cast<char*>(&file_lat), sizeof(file_lat));
            in.read(reinterpret_cast<char*>(&file_lon), sizeof(file_lon));
            if (!in) break;

            auto &cell = map.set(file_lat, file_lon);

            in.read(reinterpret_cast<char*>(&cell.code), sizeof(cell.code));
            in.read(reinterpret_cast<char*>(&cell.bioma), sizeof(cell.bioma));
            in.read(reinterpret_cast<char*>(&cell.resource), sizeof(cell.resource));
            // Load resource_production_rate vector size and data
            size_t sz = 0;
            in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
            cell.resource_production_rate.resize(sz);
            if (sz > 0)
                in.read(reinterpret_cast<char*>(cell.resource_production_rate.data()), sz * sizeof(int));
            // Load owner if present
            in.read(reinterpret_cast<char*>(&cell.c_id_owner), sizeof(cell.c_id_owner));
            in.read(reinterpret_cast<char*>(&cell.f_id_owner), sizeof(cell.f_id_owner));
            in.read(reinterpret_cast<char*>(&cell.owners), sizeof(cell.owners));
        }
    }
    in.close();
    printf("Map loaded from /tmp/saved_map.dat\n");
}


void initMap()
{
    initTiles(tiles);
    
    initCommodities(resourcesxbioma);

    map.init();

    if (preloadmap)
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
        for(int i=0;i<1000;i++)
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


        saveMap();
    }
   
    assignProductionRates(map, resources);
}




void initFactions()
{

    Faction *faction = new Faction();
    faction->id = 0;
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
    strcpy(faction->name,"Greeks");
    faction->red = 0;
    faction->green = 0;
    faction->blue = 255;
    faction->rates[0] = 1;
    faction->rates[1] = 0;
    faction->rates[2] = 0;
    faction->rates[3] = 0;
    factions.push_back(faction);


    initNaming(citynames);

}

void worldStep(int value)
{
    update(value);
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


void initWorldModelling()
{
    year = -4000;

    initFactions();   

    initUnits();

    coordinator.a_f_id = factions[0]->id;
    coordinator.a_u_id = nextUnitId(coordinator.a_f_id);
    factions[0]->autoPlayer = false;


    message(year, coordinator.a_f_id, "Sir, our destiny is to build a great empire.  We must start by building our first city.");

    centermapinmap(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);
    zoommapin();     
}