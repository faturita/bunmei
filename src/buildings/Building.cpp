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