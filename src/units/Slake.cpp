#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Slake.h"

extern Factions factions;

Slake::Slake()
{
    strcpy(name,"Slake");
    strcpy(assetname,"assets/assets/units/slake.png");
    moves = 1;
    visionRange = 1;      // scouting is the point of it: it sees twice as far as anything else
    dw = 0;
    aw = 0;
}

int Slake::getSubType()
{
    return UNIT_SCOUT;
}

int Slake::getId()
{
    return id;
}

const char* Slake::getName()
{
    return name;
}

Slake* SlakeFactory::create()
{
    return new Slake();
}

SlakeFactory::SlakeFactory()
{
    id = BUILDABLE_SLAKE;
    strncpy(this->name,"Slake",256);  
    addDependencyCode(TECH_WARRIOR_CODE);
}

std::vector<int> SlakeFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> SlakeFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}



