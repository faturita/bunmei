#include "openglutils.h"
#include "font/FontsBitmap.h"
#include "Faction.h"
#include "map.h"
#include "City.h"

extern std::vector<Faction*> factions;
extern Map map;

City::City(int pfaction, int pid, int platitude, int plongitude)
{
    strncpy(name,"Kattegat",256);

    resources.push_back(0);
    resources.push_back(0);
    resources.push_back(0);
    resources.push_back(0);
    resources.push_back(0);
    resources.push_back(0);

    isCapital = false;
    pop = 1;

    id = pid;
    faction = pfaction;
    latitude = platitude;
    longitude = plongitude;

    // We are working on the city location and one more
    // Assignment of the land to this city.
    map.peek(latitude+0, longitude+0).setCityOwnership(faction, id);
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

    coordinate c = map.to_screen(latitude,longitude);

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
                map.peek(latitude+lat, longitude+lon).setCityOwnership(new_f_id, id);
            }
        }       
}

void City::deAssigntWorkingTile()
{
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (workingOn(lat,lon) && lat!=0 && lon!=0 && numberOfWorkingTiles()>(pop+1))
            {
                map.peek(latitude+lat, longitude+lon).releaseCityOwnership();

                return;   
            }
        }    
}

void City::assignWorkingTile()
{
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (!occupied(lat, lon) && !workingOn(lat,lon) && numberOfWorkingTiles()<(pop+1)) 
            {
                map.peek(latitude+lat, longitude+lon).setCityOwnership(faction, id);
                return;   
            }
        }    
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
            map.peek(latitude+c.lat, longitude+c.lon).setCityOwnership(faction, id);
        }
        else
        {
            // Release of the land from this city
            map.peek(latitude+c.lat, longitude+c.lon).releaseCityOwnership();
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
    return map.peek(latitude+lat,longitude+lon).belongsToCity(faction, id);
}

bool City::occupied(int lat, int lon)
{

    return map.peek(latitude+lat,longitude+lon).isOccupied(faction,id);

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
                production_rate += map.peek(latitude+lat,longitude+lon).resource_production_rate[r_id];
            }
        }

    //printf("Resource: %d Production Rate: %d\n",r_id,production_rate);

    return production_rate;
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