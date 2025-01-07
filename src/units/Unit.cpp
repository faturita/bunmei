#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Unit.h"

extern Map map;
extern std::vector<Faction*> factions;

Unit::Unit()
{
    strcpy(name,"Unit");
    moves = 1;
    target = coordinate(0,0);
}

void Unit::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(oldlatitude*(1-completion)+latitude*(completion),oldlongitude*(1-completion) + longitude*(completion),16,assetname, red, green, blue);

    if (fortified)
    {
        placeThisUnit(oldlatitude*(1-completion)+latitude*(completion),oldlongitude*(1-completion) + longitude*(completion),16,"assets/assets/map/fortify.png", red, green, blue);
    }

    if (completion < 1)
        completion += 0.1;

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
    return LANDTYPE;
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

float Unit::getAttack()
{
    return aw;
}

float Unit::getDefense()
{
    return dw;
}

void Unit::update(int newlat, int newlon)
{
    oldlatitude = latitude;
    oldlongitude = longitude;

    latitude = newlat;
    longitude = newlon;

    completion = 0;
    fortified = false;
}

bool Unit::movementCompleted()
{
    return completion >= 1;
}

void Unit::fortify()
{
    fortified = true;
}

bool Unit::isFortified()
{
    return fortified;
}