#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Horseman.h"

extern std::vector<Faction*> factions;

Horseman::Horseman()
{
    strcpy(name,"Horseman");
    strcpy(assetname,"assets/assets/units/horseman.png");
    moves = 4;
    aw = 2;
}

int Horseman::getSubType()
{
    return UNIT_HORSEMAN;
}

int Horseman::getId()
{
    return id;
}

const char* Horseman::getName()
{
    return name;
}
// ----------------------------

Horseman* HorsemanFactory::create()
{
    return new Horseman();
}

HorsemanFactory::HorsemanFactory()
{
    strncpy(this->name,"Horseman",256);
    addDependencyCode(TECH_HORSEBACK_RIDING);
}

std::vector<int> HorsemanFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> HorsemanFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}

