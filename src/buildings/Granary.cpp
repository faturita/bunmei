#include "../codes.h"
#include "Granary.h"

Granary::Granary()
{
    strcpy(name,"Granary");
    strncpy(this->assetname,"assets/assets/city/granary.png",256);
    perkCodes.push_back(HALF_POPULATION_CODE);
}

int Granary::getSubType()
{
    return BUILDING_GRANARY;
}

// --------------------------------------------------------
GranaryFactory::GranaryFactory()
{
    strncpy(this->name,"Granary",256);
    addDependencyCode(TECH_POTTERY);
}

Buildable* GranaryFactory::create()
{
    Granary* b = new Granary();
    return b;
}

std::vector<int> GranaryFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> GranaryFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 50)
    {
        consumedResources.push_back(new Resource{SHIELDS, 50});
    }
    return consumedResources;
}

