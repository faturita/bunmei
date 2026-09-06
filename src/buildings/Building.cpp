#include "../resources.h"
#include "Building.h"

Building::Building()
{
    strcpy(name,"Building");
}

void Building::setName(const char* name)
{
    strncpy(this->name,name,256);
}

BuildableType Building::getType()
{
    return BuildableType::BUILDING;
}

int Building::getSubType()
{
    return -1; // Base Building has no subtype
}

std::vector<int>& Building::getPerkCodes()
{
    return perkCodes; // Base Building has no perk codes
}

int Building::getProductionRate(int r_id)
{
    return 0; // Base Building has no production rate
}

int Building::getConsumptionRate(int r_id)
{
    if (r_id == COINS)
        return 1; // Base building (generic for every building) has a cost of 1 coin
        
    return 0; // Base Building has no consumption rate for other resources
}