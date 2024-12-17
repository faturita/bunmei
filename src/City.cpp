#include "openglutils.h"
#include "map.h"
#include "City.h"

void City::draw()
{
    placeCityInMap(latitude,longitude,16,"assets/assets/map/city1.png");
}