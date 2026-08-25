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

int GranaryFactory::cost(int r_id)
{
    return 50;
}

