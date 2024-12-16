#include "../openglutils.h"
#include "../map.h"
#include "Settler.h"

void Settler::draw()
{
    placeInMap(latitude,longitude,16,"assets/assets/units/settlers.png");
}