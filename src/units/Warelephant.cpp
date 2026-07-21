#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Warelephant.h"

extern std::vector<Faction*> factions;

Warelephant::Warelephant()
{
    strcpy(name,"Warelephant");
    strcpy(assetname,"assets/assets/units/warelephant.png");
    moves = 4;
    aw = 4;
}

int Warelephant::getSubType()
{
    return UNIT_WARELEPHANT;
}
// ----------------------------

Warelephant* WarelephantFactory::create()
{
    return new Warelephant();
}

WarelephantFactory::WarelephantFactory()
{
    strncpy(this->name,"Warelephant",256);  
}

int WarelephantFactory::cost(int r_id)
{
    return 200;
}

