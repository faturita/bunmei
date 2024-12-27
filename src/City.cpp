#include "openglutils.h"
#include "font/FontsBitmap.h"
#include "Faction.h"
#include "map.h"
#include "City.h"

extern std::vector<Faction*> factions;

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

    int p = pop / 10;
    int r = pop % 10;

    std::string s;
    
    if (p>0)
    {
        s = "assets/assets/general/"+std::to_string(p)+".png";
        //placeMark(600+16*longitude-4, 0+16*latitude+1,    8,16,s.c_str());
        place(16*longitude-4,16*latitude-1,8,16, s.c_str());
        s = "assets/assets/general/"+std::to_string(r)+".png";
        //placeMark(600+16*longitude+4, 0+16*latitude+1,    8,16,s.c_str());
        place(16*longitude+4,16*latitude-1,8,16, s.c_str());
    } 
    else
    {
        s = "assets/assets/general/"+std::to_string(r)+".png";
        //placeMark(600+16*longitude, 0+16*latitude+1,    8,16,s.c_str());
        place(16*longitude,16*latitude-1,8,16, s.c_str());
    }
    placeWord((longitude-1),(latitude+1),4,8, name);
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

void City::tick()
{
    // @NOTE: This is the core game logic
    for(int i=0;i<resources.size();i++)
    {
        printf("Resource %d:%d\n",i,resources[i]);
    }

    // @FIXME: This needs to be some form of exponential progression to balance the rate of growth of the city.
    if (resources[0]>100)
    {
        resources[0] = 0;
        pop++;

        assignWorkingTile();

        // @FIXME Check what happens when there are no more tiles to work on
    }

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