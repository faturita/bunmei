#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Swordman.h"

extern std::vector<Faction*> factions;

Swordman::Swordman()
{
    strcpy(name,"Swordman");
    strcpy(assetname,"assets/assets/units/swordman.png");
    moves = 1;
    aw = 4;
}

int Swordman::getSubType()
{
    return UNIT_SWORDMAN;
}


Swordman* SwordmanFactory::create()
{
    return new Swordman();
}

SwordmanFactory::SwordmanFactory()
{
    strncpy(this->name,"Swordman",256);  
}

int SwordmanFactory::cost(int r_id)
{
    return 40;
}

