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


    for (const char* overlay : getOverlayAssets())
    {
        placeThisUnit(oldlatitude*(1-completion)+latitude*(completion),oldlongitude*(1-completion) + longitude*(completion),16,overlay, red, green, blue);
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

const char* Unit::getAssetName()
{
    return assetname;
}

std::vector<const char*> Unit::getOverlayAssets()
{
    std::vector<const char*> overlays;

    if (fortified)    overlays.push_back("assets/assets/map/fortify.png");
    if (sentried)     overlays.push_back("assets/assets/map/sentry.png");
    if (bRoading)     overlays.push_back("assets/assets/map/roading.png");
    if (bRailroading) overlays.push_back("assets/assets/map/railroading.png");
    if (bMining)      overlays.push_back("assets/assets/map/mining.png");
    if (bIrrigating)  overlays.push_back("assets/assets/map/irrigating.png");
    if (bQuarrying)   overlays.push_back("assets/assets/map/quarrying.png");
    if (bCamping)     overlays.push_back("assets/assets/map/camping.png");
    if (bDerricking)  overlays.push_back("assets/assets/map/derricking.png");
    if (bPlanting)    overlays.push_back("assets/assets/map/planting.png");

    return overlays;
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

void Unit::roading(int effort)
{
    bRoading = true;
    reqEffort = effort;
}

bool Unit::isRoading()
{
    return bRoading;
}

void Unit::mining(int effort)
{
    bMining = true;
    reqEffort = effort;
}

bool Unit::isMining()
{
    return bMining;
}

void Unit::irrigating(int effort)
{
    bIrrigating = true;
    reqEffort = effort;
}

bool Unit::isIrrigating()
{
    return bIrrigating;
}

void Unit::railroading(int effort)
{
    bRailroading = true;
    reqEffort = effort;
}

bool Unit::isRailroading()
{
    return bRailroading;
}

void Unit::quarrying(int effort)
{
    bQuarrying = true;
    reqEffort = effort;
}

bool Unit::isQuarrying()
{
    return bQuarrying;
}

void Unit::camping(int effort)
{
    bCamping = true;
    reqEffort = effort;
}

bool Unit::isCamping()
{
    return bCamping;
}

void Unit::derricking(int effort)
{
    bDerricking = true;
    reqEffort = effort;
}

bool Unit::isDerricking()
{
    return bDerricking;
}

void Unit::planting(int effort)
{
    bPlanting = true;
    reqEffort = effort;
}

bool Unit::isPlanting()
{
    return bPlanting;
}

bool Unit::isWorking()
{
    return bRoading || bMining || bIrrigating || bRailroading || bQuarrying || bCamping || bDerricking || bPlanting;
}

void Unit::completed()
{
    bRoading = false;
    bMining = false;
    bIrrigating = false;
    bRailroading = false;
    bQuarrying = false;
    bCamping = false;
    bDerricking = false;
    bPlanting = false;
}

void Unit::work()
{
    if (isWorking())
    {
        reqEffort -= (availablemoves);
        if (reqEffort <= 0)
        {
            completed();
        }
    }
}

bool Unit::workCompleted()
{
    return reqEffort <= 0;
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

void Unit::veteran()
{
    e[0] += 0.1;
    e[1] += 0.1;
    e[2] += 0.1;
    e[3] += 0.1;
    e[4] += 0.1;

    aw += 1.0;
    dw += 1.0;
    utw += 1.0;
}

int Unit::getId()
{
    return id;
}

const char* Unit::getName()
{
    return name;
}

