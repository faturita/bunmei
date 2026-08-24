#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Pretorian.h"

extern std::vector<Faction*> factions;

Pretorian::Pretorian()
{
    strcpy(name,"Pretorian");
    strcpy(assetname,"assets/assets/units/pretorian.png");
    moves = 1;
    aw = 5;
}

int Pretorian::getSubType()
{
    return UNIT_PRETORIAN;
}


Pretorian* PretorianFactory::create()
{
    return new Pretorian();
}

PretorianFactory::PretorianFactory()
{
    strncpy(this->name,"Pretorian",256);
    addDependencyCode(TECH_IRON_WORKING);
}

int PretorianFactory::cost(int r_id)
{
    return 50;
}

