#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Settler.h"


extern std::vector<Faction*> factions;

Settler::Settler()
{
    strcpy(name,"Settler");
    strcpy(assetname,"assets/assets/units/settlers.png");
    moves = 1;
    dw = 0;
    aw = 0;
}

int Settler::getSubType()
{
    return UNIT_SETTLER;
}

int Settler::getId()
{
    return id;
}

const char* Settler::getName()
{
    return name;
}
bool Settler::canBuildCity()
{
    return true;
}

int Settler::getConsumptionRate(int r_id)
{
    if (r_id == COINS)
        return 0;
    return 0; // Settlers are people fleeing so we do not need to pay them salaries.
}

Settler* SettlerFactory::create()
{
    return new Settler();
}

SettlerFactory::SettlerFactory()
{
    strncpy(this->name,"Settler",256);  
}

std::vector<int> SettlerFactory::getRequiredResources()
{
    std::vector<int> requiredResources;

    requiredResources.push_back(SHIELDS);
    requiredResources.push_back(FOOD);

    return requiredResources;
}

std::vector<Resource*> SettlerFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> fulfilledResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        if (availableResources[FOOD] && availableResources[FOOD]->amount >= 25)
        {
            fulfilledResources.push_back(new Resource{SHIELDS, 40});
            fulfilledResources.push_back(new Resource{FOOD, 25});
        }
    }

    return fulfilledResources;
}

