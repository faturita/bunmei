#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Chariot.h"

extern std::vector<Faction*> factions;

Chariot::Chariot()
{
    strcpy(name,"Chariot");
    strcpy(assetname,"assets/assets/units/chariot.png");
    moves = 4;
    dw = 1;
    aw = 3;
}

int Chariot::getSubType()
{
    return UNIT_CHARIOT;
}

int Chariot::getId()
{
    return id;
}

const char* Chariot::getName()
{
    return name;
}

Chariot* ChariotFactory::create()
{
    return new Chariot();
}

ChariotFactory::ChariotFactory()
{
    strncpy(this->name,"Chariot",256);
    addDependencyCode(TECH_THE_WHEEL);
}

int ChariotFactory::cost(int r_id)
{
    return 40;
}



