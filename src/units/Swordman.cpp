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

// Swordman requires 40 shields, plus 25 iron OR 25 copper.
std::vector<int> SwordmanFactory::getRequiredResources()
{
    std::vector<int> requiredResources;

    requiredResources.push_back(SHIELDS);
    requiredResources.push_back(iron);
    requiredResources.push_back(copper);

    return requiredResources;
}

std::vector<Resource*> SwordmanFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> fulfilledResources;

    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        if (availableResources[iron] && availableResources[iron]->amount >= 25)
        {
            fulfilledResources.push_back(new Resource{SHIELDS, 40});
            fulfilledResources.push_back(new Resource{iron, 25});
        }
        else if (availableResources[copper] && availableResources[copper]->amount >= 25)
        {
            fulfilledResources.push_back(new Resource{SHIELDS, 40});
            fulfilledResources.push_back(new Resource{copper, 25});
        }
    }

    return fulfilledResources;
}

