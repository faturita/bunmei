#include "openglutils.h"
#include "font/FontsBitmap.h"
#include "Faction.h"
#include "map.h"
#include "tiles.h"
#include "City.h"

extern std::vector<Faction*> factions;
extern std::unordered_map<int,int> commodityxresource;
extern ImprovementResources improvementresources;


int getPopulationThresshold(int pop)
{
    return 100*pop;
}


City::City(Map *mn, int pfaction, int pid, int platitude, int plongitude)
{
    strncpy(name,"Kattegat",256);

    map = mn;

    resources.clear();          // Core resources, commodities and manufactured goods -- one stockpile.

    for (int i=0;i<6;i++)
    {
        resources[ALL_CORE_RESOURCES[i]] = 0;
    }

   for (int i=0;i<sizeof(ALL_COMMODITIES)/sizeof(int);i++)
    {
        resources[ALL_COMMODITIES[i]] = 0;
    }

    for (int i=0;i<sizeof(ALL_MFG_GOODS)/sizeof(int);i++)
    {
        resources[ALL_MFG_GOODS[i]] = 0;
    }

    isCapital = false;
    pop = 1;

    id = pid;
    faction = pfaction;
    latitude = platitude;
    longitude = plongitude;

    // We are working on the city location and one more
    // Assignment of the land to this city.
    map->peek(latitude+0, longitude+0).setCityOwnership(faction, id);
    assignWorkingTile();

}

void City::setName(const char* name)
{
    strncpy(this->name,name,256);
}   

void City::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisCity(latitude,longitude, red,green,blue);

    if (isDefended)
    {
        placeThisTile(latitude,longitude,16,"assets/assets/map/defended.png");
    }

    coordinate c = map->to_screen(latitude,longitude);

    int lon = c.lon;
    int lat = c.lat;

    int p = pop / 10;
    int r = pop % 10;

    std::string s;
    
    if (p>0)
    {
        s = "assets/assets/general/"+std::to_string(p)+".png";
        //placeMark(600+16*longitude-4, 0+16*latitude+1,    8,16,s.c_str());
        place(16*lon-3,16*lat-1,8,16, s.c_str());
        s = "assets/assets/general/"+std::to_string(r)+".png";
        //placeMark(600+16*longitude+4, 0+16*latitude+1,    8,16,s.c_str());
        place(16*lon+3,16*lat-1,8,16, s.c_str());
    } 
    else
    {
        s = "assets/assets/general/"+std::to_string(r)+".png";
        //placeMark(600+16*longitude, 0+16*latitude+1,    8,16,s.c_str());
        place(16*lon,16*lat-1,8,16, s.c_str());
    }
    placeWord((lon-1),(lat+1),4,8, name);
}

int City::numberOfWorkingTiles()
{
    int workingTiles = 0;
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (workingOn(lat,lon))
            {
                workingTiles++;
            }
        }       
    return workingTiles;
}

void City::reAssignWorkingTiles(int new_f_id)
{
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (workingOn(lat,lon))
            {
                map->peek(latitude+lat, longitude+lon).setCityOwnership(new_f_id, id);
            }
        }       
}

// Gives up ONE tile the city is no longer entitled to work, and nothing when it is inside its
// pop+1 allowance. Which tile is not chosen by anything -- it is the first one the scan finds.
//
// The guard used to read `lat!=0 && lon!=0`, which skips every tile on the centre ROW OR
// COLUMN rather than just the centre: (1,0), (0,1), (2,0), (0,-3) and nine others could never
// be released, so a shrinking city whose surplus happened to sit on an axis kept working more
// than pop+1 forever. Only the centre (0,0) -- the city's own tile, always worked -- is
// exempt.
void City::deAssignWorkingTile()
{
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (workingOn(lat,lon) && !(lat==0 && lon==0) && numberOfWorkingTiles()>(pop+1))
            {
                map->peek(latitude+lat, longitude+lon).releaseCityOwnership();

                return;
            }
        }
}

// The city's working-tile allowance: the centre plus one per population point. numberOfWorkingTiles()
// counts the centre, so this is the number to compare it against directly.
int City::workingTileAllowance()
{
    return pop + 1;
}

// ---- explicit, non-toggling tile assignment ------------------------------------------------
// assignWorkingTile(coordinate) is a TOGGLE: it assigns or releases depending on what the
// tile currently is, which is right for a UI click but wrong for anything that has to state
// its intent -- the AI, and a remote player whose view of the city may be a turn stale. These
// two say exactly what they want and do nothing if it is already so.

// Puts this city's workforce on one specific tile. Does NOTHING (returns false) when the tile
// is already worked, when the city has no assignment left (numberOfWorkingTiles() has reached
// its allowance), when the tile is out of the 7x7 range, when it is the centre (always worked,
// never assignable) or when another faction or city holds it.
bool City::assignTile(coordinate c)
{
    if (c.lat < -3 || c.lat > 3 || c.lon < -3 || c.lon > 3)
        return false;
    if (c.lat == 0 && c.lon == 0)
        return false;                                   // the city's own tile
    if (workingOn(c.lat, c.lon))
        return false;                                   // already assigned
    if (numberOfWorkingTiles() >= workingTileAllowance())
        return false;                                   // no assignment left
    if (occupied(c.lat, c.lon))
        return false;                                   // someone else's land

    map->peek(latitude+c.lat, longitude+c.lon).setCityOwnership(faction, id);
    return true;
}

// Takes this city's workforce off one specific tile. Does NOTHING (returns false) when the
// tile is not currently worked by this city, when it is out of range, or when it is the
// centre. Unlike assigning, this is never limited by the allowance -- giving up a tile can
// only ever bring the city further inside it.
bool City::deAssignTile(coordinate c)
{
    if (c.lat < -3 || c.lat > 3 || c.lon < -3 || c.lon > 3)
        return false;
    if (c.lat == 0 && c.lon == 0)
        return false;                                   // the city's own tile
    if (!workingOn(c.lat, c.lon))
        return false;                                   // not assigned: nothing to give up

    map->peek(latitude+c.lat, longitude+c.lon).releaseCityOwnership();
    return true;
}

void City::assignWorkingTile()
{
    // @NOTE: We are maximizing food production
    coordinate max_food_production_tile(0,0);
    int max = 0;

    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (!occupied(lat, lon) && !workingOn(lat,lon) && numberOfWorkingTiles()<(pop+1)) 
            {
                int production = map->peek(latitude+lat, longitude+lon).getResourceProductionRate(FOOD);
                if (production>max)
                {
                    max = production;
                    max_food_production_tile = coordinate(lat,lon);
                }
            }
        }   

    assignWorkingTile(max_food_production_tile);


    //for(int lat=-3;lat<=3;lat++)
    //    for(int lon=-3;lon<=3;lon++)
    //    {
    //        if (!occupied(lat, lon) && !workingOn(lat,lon) && numberOfWorkingTiles()<(pop+1)) 
    //        {
    //            map->peek(latitude+lat, longitude+lon).setCityOwnership(faction, id);
    //            return;   
    //        }
    //    }    
}

void City::assignWorkingTile(coordinate c)
{
    if (c.lat < -3 || c.lat > 3 || c.lon < -3 || c.lon > 3 || (c.lat == 0 && c.lon == 0))
    {
        return;
    }

    if (!occupied(c.lat, c.lon))
    {
        if (!workingOn(c.lat,c.lon) && numberOfWorkingTiles()<(pop+1))        // Everybody can work on the fields (on the available fields)
        {
            // Assignment of the land to this city.
            map->peek(latitude+c.lat, longitude+c.lon).setCityOwnership(faction, id);
        }
        else
        {
            // Release of the land from this city
            map->peek(latitude+c.lat, longitude+c.lon).releaseCityOwnership();
        }
    }
}

// Lat Lon are RELATIVE to the city here.
// bool City::workingOn(int lat, int lon)
// {
//     // @NOTE: Eventually we can add who is working (professions)
//     coordinate c(lat,lon);

//     if (tiles.find(c) != tiles.end())
//     {
//         return true;
//     }

//     return false;
// }

// Lat Lon are RELATIVE to the city here.
bool City::workingOn(int lat, int lon)
{
    return map->peek(latitude+lat,longitude+lon).belongsToCity(faction, id);
}

bool City::occupied(int lat, int lon)
{

    return map->peek(latitude+lat,longitude+lon).isOccupied(faction,id);

}


bool City::isCapitalCity()
{
    return isCapital;
}

void City::setCapitalCity()
{
    isCapital = true;
}

int City::getProductionRate(int r_id)
{
    int production_rate = 0;
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (workingOn(lat,lon))
            {
                production_rate += map->peek(latitude+lat,longitude+lon).getResourceProductionRate(r_id);
            }
        }

    //printf("Resource: %d Production Rate: %d\n",r_id,production_rate);

    return production_rate;
}

// Commodities are gathered from every tile in the city's working RANGE (same 7x7 bounds as
// getProductionRate above), regardless of whether the tile is actually assigned/worked: one
// unit of the matching commodity per special-resource tile, or zero if that resource needs
// an improvement (initImprovementResources/getRequiredImprovement, tiles.cpp) that has not
// been built on the tile yet.
int City::getCommodityProductionRate(int commodity_id)
{
    int rate = 0;
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            mapcell &cell = map->peek(latitude+lat, longitude+lon);

            if (cell.resource == 0)
                continue;

            // Gather resources for this city only if it is free land or if it belongs to this city.
            if (!cell.isFreeLand() && (cell.c_id_owner != id || cell.f_id_owner != faction))
                continue;

            auto it = commodityxresource.find(cell.resource);
            if (it == commodityxresource.end() || it->second != commodity_id)
                continue;

            int required = getRequiredImprovement(improvementresources, cell.resource);
            if (required != 0 && (cell.improvements & required) != required)
                continue;

            rate++;
        }

    return rate;
}

int City::getConsumptionRate(int r_id)
{
    // @FIXME: Add the consumption rate for each unit that belong to the city and so on.
    int consumption_rate = 0;
    switch (r_id)
    {
    case 0: // Food
        consumption_rate = pop*2;
        break;

    default:
        break;
    }

    return consumption_rate;
}

int City::getBuildingConsumptionRate(int r_id)
{
    int rate = 0;
    for (Building* b : buildings)
        rate += b->getConsumptionRate(r_id);
    return rate;
}

void City::setDefense()
{
    isDefended = true;
}

void City::noDefense()
{
    isDefended = false;
}

bool City::isDefendedCity()
{
    return isDefended;
}

coordinate City::getCoordinate()
{
    return coordinate(latitude,longitude);
}