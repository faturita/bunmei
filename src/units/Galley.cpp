#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Galley.h"

extern std::vector<Faction*> factions;

Galley::Galley()
{
    strcpy(name,"Galley");
    moves = 4;
}

void Galley::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,15,"assets/assets/units/galley.png", red, green, blue);
}

bool Galley::canBuildCity()
{
    return false;
}

MOVEMENT_TYPE Galley::getMovementType()
{
    return OCEANTYPE;
}

// ----------------------------

Galley* GalleyFactory::create()
{
    return new Galley();
}

GalleyFactory::GalleyFactory()
{
    strncpy(this->name,"Galley",256);  
}

int GalleyFactory::cost(int r_id)
{
    return 40;
}