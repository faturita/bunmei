#include "../codes.h"
#include "Barracks.h"

Barracks::Barracks()
{
    strcpy(name,"Barracks");
    strncpy(this->assetname,"assets/assets/city/barracks.png",256);
    perkCodes.push_back(VETERAN_CODE);
}

int Barracks::getSubType()
{
    return BUILDING_BARRACKS;
}

// --------------------------------------------------------

BarracksFactory::BarracksFactory()
{
    strncpy(this->name,"Barracks",256);
    addDependencyCode(TECH_WARRIOR_CODE);
}

Buildable* BarracksFactory::create()
{
    Barracks* b = new Barracks();
    return b;
}

std::vector<int> BarracksFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> BarracksFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 50)
    {
        consumedResources.push_back(new Resource{SHIELDS, 50});
    }
    return consumedResources;
}

