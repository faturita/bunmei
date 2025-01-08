#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Galley.h"

extern std::vector<Faction*> factions;

Galley::Galley()
{
    strcpy(name,"Galley");
    strcpy(assetname,"assets/assets/units/galley.png");
    moves = 4;
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