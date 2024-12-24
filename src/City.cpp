#include "openglutils.h"
#include "map.h"
#include "City.h"

City::City()
{
    strncpy(name,"Kattegat",256);
}

void City::draw()
{
    placeThisCity(latitude,longitude);

    int p = pop / 10;
    int r = pop % 10;

    std::string s;
    
    if (p>0)
    {
        s = "assets/assets/general/"+std::to_string(p)+".png";
        //placeMark(600+16*longitude-4, 0+16*latitude+1,    8,16,s.c_str());
        place(16*longitude-4,16*latitude+1,8,16, s.c_str());
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
}

void City::tick()
{
    int seasonshields = 0;
    for(int lat=-2;lat<=2;lat++)
        for(int lon=-2;lon<=2;lon++)
        {

        }
}