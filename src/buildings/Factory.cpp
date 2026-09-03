#include "../codes.h"
#include "../resources.h"
#include "Factory.h"

Factory::Factory()
{
    strcpy(name,"Factory");
    strncpy(this->assetname,"assets/assets/city/factory.png",256);
}

int Factory::getSubType()
{
    return BUILDING_FACTORY;
}

int Factory::getProductionRate(int r_id)
{
    if (r_id == tools)
    {
        return 1;
    }
    return 0; // Default production rate for other resources
}

int Factory::getConsumptionRate(int r_id)
{
    if (r_id == iron)
    {
        return 1;
    }
    return 0; // Default consumption rate for other resources
}

// --------------------------------------------------------
FactoryFactory::FactoryFactory()
{
    strncpy(this->name,"Factory",256);
    addDependencyCode(TECH_INDUSTRIALIZATION);
}

Buildable* FactoryFactory::create()
{
    Factory* b = new Factory();
    return b;
}

int FactoryFactory::cost(int r_id)
{
    return 200;
}

