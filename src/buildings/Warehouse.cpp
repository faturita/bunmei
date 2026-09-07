#include "../codes.h"
#include "Warehouse.h"

Warehouse::Warehouse()
{
    strcpy(name,"Warehouse");
    strncpy(this->assetname,"assets/assets/city/warehouse.png",256);
    perkCodes.push_back(STORAGE_EXPANSION_1);
}

int Warehouse::getSubType()
{
    return BUILDING_WAREHOUSE;
}

// --------------------------------------------------------
WarehouseFactory::WarehouseFactory()
{
    strncpy(this->name,"Warehouse",256);
    addDependencyCode(TECH_POTTERY);
}

Buildable* WarehouseFactory::create()
{
    Warehouse* b = new Warehouse();
    return b;
}

std::vector<int> WarehouseFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> WarehouseFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 100)
    {
        consumedResources.push_back(new Resource{SHIELDS, 100});
    }
    return consumedResources;
}

