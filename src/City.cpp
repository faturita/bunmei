#include "openglutils.h"
#include "font/FontsBitmap.h"
#include "Faction.h"
#include "map.h"
#include "City.h"

extern std::vector<Faction*> factions;
extern Map map;

City::City()
{
    strncpy(name,"Kattegat",256);

    resources.push_back(0);
    resources.push_back(0);
    resources.push_back(0);
    resources.push_back(0);
    resources.push_back(0);


    tiles[coordinate(0,0)] = 1;     // We are working on the city location.
    assignWorkingTile();

    isCapital = false;

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

    coordinate c = map.to_fixed(latitude,longitude);

    int lon = c.lon;
    int lat = c.lat;

    int p = pop / 10;
    int r = pop % 10;

    std::string s;
    
    if (p>0)
    {
        s = "assets/assets/general/"+std::to_string(p)+".png";
        //placeMark(600+16*longitude-4, 0+16*latitude+1,    8,16,s.c_str());
        place(16*lon-4,16*lat-1,8,16, s.c_str());
        s = "assets/assets/general/"+std::to_string(r)+".png";
        //placeMark(600+16*longitude+4, 0+16*latitude+1,    8,16,s.c_str());
        place(16*lon+4,16*lat-1,8,16, s.c_str());
    } 
    else
    {
        s = "assets/assets/general/"+std::to_string(r)+".png";
        //placeMark(600+16*longitude, 0+16*latitude+1,    8,16,s.c_str());
        place(16*lon,16*lat-1,8,16, s.c_str());
    }
    placeWord((lon-1),(lat+1),4,8, name);
}

void City::assignWorkingTile()
{
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (!workingOn(lat,lon))
            {
                tiles[coordinate(lat,lon)] = 1;
                return;   
            }
        }    
}

void City::deAssigntWorkingTile()
{
    for(int lat=-3;lat<=3;lat++)
        for(int lon=-3;lon<=3;lon++)
        {
            if (workingOn(lat,lon) && lat!=0 && lon!=0 && tiles.size()>(pop+1))
            {
                tiles.erase(coordinate(lat,lon));
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

    if (!workingOn(c.lat,c.lon) && tiles.size()<(pop+1))        // Everybody can work on the fields (on the available fields)
        tiles[c] = 1;
    else
        tiles.erase(c);
}

// Lat Lon are RELATIVE to the city here.
bool City::workingOn(int lat, int lon)
{
    // @NOTE: Eventually we can add who is working (professions)
    coordinate c(lat,lon);

    if (tiles.find(c) != tiles.end())
    {
        return true;
    }

    return false;

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
                production_rate += map(latitude+lat,longitude+lon).resource_production_rate[r_id];
            }
        }

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