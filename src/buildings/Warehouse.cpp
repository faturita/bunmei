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

int WarehouseFactory::cost(int r_id)
{
    return 100;
}

