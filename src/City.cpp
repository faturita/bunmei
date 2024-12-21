#include "openglutils.h"
#include "map.h"
#include "City.h"

void City::draw()
{
    //placeCityInMap(latitude,longitude,16,"assets/assets/map/city_r.png");

    int p = pop / 10;
    int r = pop % 10;

    std::string s;
    
    if (p>0)
    {
        s = "assets/assets/general/"+std::to_string(p)+".png";
        placeMark(600+16*longitude-4, 0+16*latitude+1,    8,16,s.c_str());
        s = "assets/assets/general/"+std::to_string(r)+".png";
        placeMark(600+16*longitude+4, 0+16*latitude+1,    8,16,s.c_str());
    } 
    else
    {
        s = "assets/assets/general/"+std::to_string(r)+".png";
        placeMark(600+16*longitude, 0+16*latitude+1,    8,16,s.c_str());
    }



}