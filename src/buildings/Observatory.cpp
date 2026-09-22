#include "../codes.h"
#include "Observatory.h"

Observatory::Observatory()
{
    strcpy(name,"Observatory");
    strncpy(this->assetname,"assets/assets/city/observatory.png",256);
    perkCodes.push_back(SCIENCE_SURPLUS_CODE);
}

int Observatory::getSubType()
{
    return BUILDING_OBSERVATORY;
}

// --------------------------------------------------------
ObservatoryFactory::ObservatoryFactory()
{
    id = BUILDABLE_OBSERVATORY;
    strncpy(this->name,"Observatory",256);
    addDependencyCode(TECH_ASTRONOMY);
}

Buildable* ObservatoryFactory::create()
{
    Observatory* b = new Observatory();
    return b;
}

std::vector<int> ObservatoryFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> ObservatoryFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 200)
    {
        consumedResources.push_back(new Resource{SHIELDS, 200});
    }
    return consumedResources;
}

