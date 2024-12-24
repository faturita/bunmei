#include "../openglutils.h"
#include "../map.h"
#include "Settler.h"

void Settler::draw()
{
    placeThisUnit(latitude,longitude,16,"assets/assets/units/settlers.png");
}