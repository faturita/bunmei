#include "../openglutils.h"
#include "../map.h"
#include "Warrior.h"


void Warrior::draw()
{
    placeInMap(longitude,latitude,16,"assets/assets/units/warrior.png");
}