#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Trireme.h"

extern std::vector<Faction*> factions;

Trireme::Trireme()
{
    strcpy(name,"Trireme");
    strcpy(assetname,"assets/assets/units/trireme.png");
    moves = 4;
    aw = 2;
}


bool Trireme::canBuildCity()
{
    return false;
}

MOVEMENT_TYPE Trireme::getMovementType()
{
    return OCEANTYPE;
}

// ----------------------------

Trireme* TriremeFactory::create()
{
    return new Trireme();
}

TriremeFactory::TriremeFactory()
{
    strncpy(this->name,"Trireme",256);  
}

int TriremeFactory::cost(int r_id)
{
    return 40;
}