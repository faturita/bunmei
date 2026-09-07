#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Spearman.h"

extern std::vector<Faction*> factions;

Spearman::Spearman()
{
    strcpy(name,"Spearman");
    strcpy(assetname,"assets/assets/units/spearman.png");
    moves = 1;
    dw = 3;
}

int Spearman::getSubType()
{
    return UNIT_SPEARMAN;
}

int Spearman::getId()
{
    return id;
}

const char* Spearman::getName()
{
    return name;
}

Spearman* SpearmanFactory::create()
{
    return new Spearman();
}

SpearmanFactory::SpearmanFactory()
{
    strncpy(this->name,"Spearman",256);
    addDependencyCode(TECH_WARRIOR_CODE);
}

std::vector<int> SpearmanFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> SpearmanFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}



