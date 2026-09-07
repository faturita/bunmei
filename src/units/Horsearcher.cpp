#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Horsearcher.h"

extern std::vector<Faction*> factions;

Horsearcher::Horsearcher()
{
    strcpy(name,"Horsearcher");
    strcpy(assetname,"assets/assets/units/horsearcher.png");
    moves = 4;
    aw = 2;
}

int Horsearcher::getSubType()
{
    return UNIT_HORSEARCHER;
}

int Horsearcher::getId()
{
    return id;
}

const char* Horsearcher::getName()
{
    return name;
}

// ----------------------------

Horsearcher* HorsearcherFactory::create()
{
    return new Horsearcher();
}

HorsearcherFactory::HorsearcherFactory()
{
    strncpy(this->name,"Horsearcher",256);
    addDependencyCode(TECH_ANIMAL_HUSBANDRY);
    addDependencyCode(TECH_HORSEBACK_RIDING);
}

std::vector<int> HorsearcherFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> HorsearcherFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}

