#include "../codes.h"
#include "../resources.h"
#include "Depot.h"

Depot::Depot()
{
    strcpy(name,"Depot");
    strncpy(this->assetname,"assets/assets/city/depot.png",256);
    perkCodes.push_back(STORAGE_EXPANSION_1);
}

int Depot::getSubType()
{
    return BUILDING_DEPOT;
}

// --------------------------------------------------------
DepotFactory::DepotFactory()
{
    strncpy(this->name,"Depot",256);
    addDependencyCode(TECH_POTTERY);
}

Buildable* DepotFactory::create()
{
    Depot* b = new Depot();
    return b;
}

int DepotFactory::cost(int r_id)
{
    if (r_id == SHIELDS)
    {
        return 100;
    }
    else if (r_id == tools)
    {
        return 100;
    }
    else {
        return 0;
    }
}

