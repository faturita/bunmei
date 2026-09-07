#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Pretorian.h"

extern std::vector<Faction*> factions;

Pretorian::Pretorian()
{
    strcpy(name,"Pretorian");
    strcpy(assetname,"assets/assets/units/pretorian.png");
    moves = 1;
    aw = 5;
}

int Pretorian::getSubType()
{
    return UNIT_PRETORIAN;
}

int Pretorian::getId()
{
    return id;
}

const char* Pretorian::getName()
{
    return name;
}


Pretorian* PretorianFactory::create()
{
    return new Pretorian();
}

PretorianFactory::PretorianFactory()
{
    strncpy(this->name,"Pretorian",256);
    addDependencyCode(TECH_IRON_WORKING);
}

std::vector<int> PretorianFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> PretorianFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 50)
    {
        consumedResources.push_back(new Resource{SHIELDS, 50});
    }
    return consumedResources;
}

