#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Scout.h"

extern std::vector<Faction*> factions;

Scout::Scout()
{
    strcpy(name,"Scout");
    strcpy(assetname,"assets/assets/units/scout.png");
    moves = 3;
    dw = 0;
    aw = 0;
}

int Scout::getSubType()
{
    return UNIT_SCOUT;
}

Scout* ScoutFactory::create()
{
    return new Scout();
}

ScoutFactory::ScoutFactory()
{
    strncpy(this->name,"Scout",256);  
}

int ScoutFactory::cost(int r_id)
{
    return 40;
}



