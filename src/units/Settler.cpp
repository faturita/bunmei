#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Settler.h"


extern std::vector<Faction*> factions;

Settler::Settler()
{
    strcpy(name,"Settler");
    strcpy(assetname,"assets/assets/units/settlers.png");
    moves = 1;
    dw = 0;
    aw = 0;
}

int Settler::getSubType()
{
    return UNIT_SETTLER;
}

int Settler::getId()
{
    return id;
}

const char* Settler::getName()
{
    return name;
}
bool Settler::canBuildCity()
{
    return true;
}

int Settler::getConsumptionRate(int r_id)
{
    if (r_id == COINS)
        return 0;
    return 0; // Settlers are people fleeing so we do not need to pay them salaries.
}

Settler* SettlerFactory::create()
{
    return new Settler();
}

SettlerFactory::SettlerFactory()
{
    strncpy(this->name,"Settler",256);  
}

int SettlerFactory::cost(int r_id)
{
    // @FIXME: Add also the cost in terms of food (and modify the loop to include all the required resources)
    return 100;
}

