#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Spy.h"

extern std::vector<Faction*> factions;

Spy::Spy()
{
    strcpy(name,"Spy");
    strcpy(assetname,"assets/assets/units/spy.png");
    moves = 3;
    dw = 0;
    aw = 0;
}

int Spy::getSubType()
{
    return UNIT_SPY;
}

Spy* SpyFactory::create()
{
    return new Spy();
}

SpyFactory::SpyFactory()
{
    strncpy(this->name,"Spy",256);  
}

int SpyFactory::cost(int r_id)
{
    return 80;
}



