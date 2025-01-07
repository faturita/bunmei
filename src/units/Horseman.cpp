#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Horseman.h"

extern std::vector<Faction*> factions;

Horseman::Horseman()
{
    strcpy(name,"Horseman");
    strcpy(assetname,"assets/assets/units/horseman.png");
    moves = 4;
    aw = 2;
}


bool Horseman::canBuildCity()
{
    return false;
}

// ----------------------------

Horseman* HorsemanFactory::create()
{
    return new Horseman();
}

HorsemanFactory::HorsemanFactory()
{
    strncpy(this->name,"Horseman",256);  
}

int HorsemanFactory::cost(int r_id)
{
    return 40;
}