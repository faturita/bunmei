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

std::vector<int> ChariotFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> ChariotFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}



