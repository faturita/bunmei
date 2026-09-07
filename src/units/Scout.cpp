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

int Scout::getId()
{
    return id;
}

const char* Scout::getName()
{
    return name;
}

Scout* ScoutFactory::create()
{
    return new Scout();
}

ScoutFactory::ScoutFactory()
{
    strncpy(this->name,"Scout",256);  
}

std::vector<int> ScoutFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> ScoutFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}



