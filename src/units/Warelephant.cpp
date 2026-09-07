#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Warelephant.h"

extern std::vector<Faction*> factions;

Warelephant::Warelephant()
{
    strcpy(name,"Warelephant");
    strcpy(assetname,"assets/assets/units/warelephant.png");
    moves = 4;
    aw = 4;
}

int Warelephant::getSubType()
{
    return UNIT_WARELEPHANT;
}

int Warelephant::getId()
{
    return id;
}

const char* Warelephant::getName()
{
    return name;
}
// ----------------------------

Warelephant* WarelephantFactory::create()
{
    return new Warelephant();
}

WarelephantFactory::WarelephantFactory()
{
    strncpy(this->name,"Warelephant",256);
    addDependencyCode(TECH_ANIMAL_HUSBANDRY);
}

std::vector<int> WarelephantFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> WarelephantFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 200)
    {
        consumedResources.push_back(new Resource{SHIELDS, 200});
    }
    return consumedResources;
}

