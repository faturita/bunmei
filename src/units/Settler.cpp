#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Settler.h"


extern std::vector<Faction*> factions;

Settler::Settler()
{
    strcpy(name,"Settler");
    strcpy(assetname,"assets/assets/units/settlers.png");
    moves = 2;
    dw = 0;
    aw = 0;
}

int Settler::getSubType()
{
    return UNIT_SETTLER;
}
bool Settler::canBuildCity()
{
    return true;
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

