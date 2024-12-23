#include "../openglutils.h"
#include "../map.h"
#include "Unit.h"

extern Map map;

void Unit::draw()
{
    placeThisUnit(longitude,latitude,16,"assets/assets/units/tank.png");
}
