#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
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

int Swordman::getId()
{
    return id;
}

const char* Swordman::getName()
{
    return name;
}


Swordman* SwordmanFactory::create()
{
    return new Swordman();
}

SwordmanFactory::SwordmanFactory()
{
    strncpy(this->name,"Swordman",256);
    addDependencyCode(TECH_IRON_WORKING);
}

int SwordmanFactory::cost(int r_id)
{
    // Mmmmm i need a regexp an FDA to map differnt type of requirements for each element.  For instance this requires 40 shields & 20 coins and (20 iron or 10 copper)
    if (r_id == SHIELDS)
        return 40;
    else if (r_id == iron)
        return 25;

    return 0; // Default cost for other resources
}

