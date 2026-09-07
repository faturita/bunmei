#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Galley.h"

extern std::vector<Faction*> factions;

Galley::Galley()
{
    strcpy(name,"Galley");
    strcpy(assetname,"assets/assets/units/galley.png");
    moves = 4;
}

int Galley::getSubType()
{
    return UNIT_GALLEY;
}

MOVEMENT_TYPE Galley::getMovementType()
{
    return OCEANTYPE;
}

// ----------------------------

Galley* GalleyFactory::create()
{
    return new Galley();
}

GalleyFactory::GalleyFactory()
{
    strncpy(this->name,"Galley",256);
    // README.md:126 requires "Map Making + Horseback Riding".
    addDependencyCode(TECH_MAP_MAKING);
    addDependencyCode(TECH_HORSEBACK_RIDING);
}

std::vector<int> GalleyFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> GalleyFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}

