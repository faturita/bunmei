#include "../openglutils.h"
#include "../map.h"
#include "Unit.h"

extern Map map;

Unit::Unit()
{
    strcpy(name,"Unit");
    moves = 1;
}

void Unit::draw()
{
    placeThisUnit(longitude,latitude,16,"assets/assets/units/tank.png");
}

int Unit::getUnitMoves()
{
    return moves;
}

bool Unit::canBuildCity()
{
    return false;
}

int Unit::cost(int r_id)
{
    return 100;
}

BuildableType Unit::getType()
{
    return BuildableType::UNIT;
}

MOVEMENT_TYPE Unit::getMovementType()
{
    return LAND;
}