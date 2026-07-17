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

    if (bDestroy)
    {
        char strcombat[256];
        sprintf(strcombat,"assets/assets/units/combat_%d.png",(int)(completion*10));
        placeThisUnit(oldlatitude*(1-completion)+latitude*(completion),oldlongitude*(1-completion) + longitude*(completion),16,strcombat, red, green, blue); 
    }


    if (fortified)
    {
        placeThisUnit(oldlatitude*(1-completion)+latitude*(completion),oldlongitude*(1-completion) + longitude*(completion),16,"assets/assets/map/fortify.png", red, green, blue);
    }

    if (sentried)
    {
        placeThisUnit(oldlatitude*(1-completion)+latitude*(completion),oldlongitude*(1-completion) + longitude*(completion),16,"assets/assets/map/sentry.png", red, green, blue);
    }

    if (completion < 1)
        completion += 0.1;

    if (movementCompleted() && goBack)
    {
        latitude = oldlatitude;
        longitude = oldlongitude;
        goBack = false;
    }

    if (markedForDeletion && !goBack && completion <1 && completion >0.9)
    {
        completion = 0;
        latitude = oldlatitude;
        longitude = oldlongitude;
        goBack = true;
        bDestroy = true;
    }



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
    // Normalize the target into real map coordinates (the spheroid wraps),
    // so pathfinding lookups and arrived() compare against the same values.
    coordinate c = map.adjust(lat,lon,0,0);
    printf("GoTo: Target (Lat,Lon)=(%d, %d)\n",c.lat,c.lon);
    autoMode = true;
    target = c;
}

void Unit::resetGoTo()
{
    autoMode = false;
    target = coordinate(latitude,longitude);
}

bool Unit::isAuto()
{
    return autoMode;
}

void Unit::setPendingMove(coordinate c)
{
    haspendingmove = true;
    pendingmove = c;
}

bool Unit::hasPendingMove()
{
    return haspendingmove;
}

coordinate Unit::getPendingMove()
{
    return pendingmove;
}

void Unit::clearPendingMove()
{
    haspendingmove = false;
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

void Unit::packUp()
{
    fortified = false;
}

void Unit::sentry()
{
    sentried = true;
}

bool Unit::isSentry()
{
    return sentried;
}

void Unit::wakeUp()
{
    sentried = false;
}

// Dying units (killed in battle) stay in the units map until their animation completes:
// they must not be selectable as the active unit.
bool Unit::isDying()
{
    return markedForDeletion;
}

bool Unit::isMarkedForDeletion()
{
    return (markedForDeletion && movementCompleted());
}

void Unit::markForDeletion()
{
    markedForDeletion = true;
}

void Unit::destroy()
{
    markedForDeletion = true;
    bDestroy = true;
    completion = 0;

    oldlatitude = latitude;
    oldlongitude = longitude;
}

void Unit::goBackOnCompletion()
{
    goBack = true;
}

int Unit::getSubType()
{
    return -1;
}

