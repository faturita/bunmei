#include "building.h"

void Building::setName(const char* name)
{
    strncpy(this->name,name,256);
}

int Building::cost(int r_id)
{
    return 100;
}

BuildableType Building::getType()
{
    return BuildableType::BUILDING;
}