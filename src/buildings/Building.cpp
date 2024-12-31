#include "building.h"

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