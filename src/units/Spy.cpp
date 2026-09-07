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

int Spy::getId()
{
    return id;
}

const char* Spy::getName()
{
    return name;
}

Spy* SpyFactory::create()
{
    return new Spy();
}

SpyFactory::SpyFactory()
{
    strncpy(this->name,"Spy",256);  
}

std::vector<int> SpyFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> SpyFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 80)
    {
        consumedResources.push_back(new Resource{SHIELDS, 80});
    }
    return consumedResources;
}



