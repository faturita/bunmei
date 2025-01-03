#include "../openglutils.h"
#include "../map.h"
#include "Unit.h"

extern Map map;

Unit::Unit()
{
    strcpy(name,"Unit");
    moves = 1;
    target = coordinate(0,0);
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

void Unit::goTo(int lat, int lon)
{
    printf("Target Lat Lon %d, %d\n",lat,lon);
    autoMode = true;
    target = coordinate(lat,lon);
}

bool Unit::isAuto()
{
    return autoMode;
}

bool Unit::arrived()
{
    if (latitude == target.lat && longitude == target.lon)
    {
        autoMode = false;
        return true;
    }

    return false;
}

coordinate Unit::getCoordinate()
{
    return coordinate(latitude,longitude);
}