#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Axeman.h"

extern std::vector<Faction*> factions;

Axeman::Axeman()
{
    strcpy(name,"Axeman");
    strcpy(assetname,"assets/assets/units/axeman.png");
    moves = 1;
    dw = 1;
    aw = 3;
}

int Axeman::getSubType()
{
    return UNIT_AXEMAN;
}

int Axeman::getId()
{
    return id;
}

const char* Axeman::getName()
{
    return name;
}


Axeman* AxemanFactory::create()
{
    return new Axeman();
}

AxemanFactory::AxemanFactory()
{
    strncpy(this->name,"Axeman",256);
    addDependencyCode(TECH_IRON_WORKING);
}

int AxemanFactory::cost(int r_id)
{
    return 40;
}

