#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Archer.h"

extern std::vector<Faction*> factions;

Archer::Archer()
{
    strcpy(name,"Archer");
    strcpy(assetname,"assets/assets/units/archer.png");
    moves = 1;
    dw = 2;
}

int Archer::getSubType()
{
    return UNIT_ARCHER;
}

int Archer::getId()
{
    return id;
}

const char* Archer::getName()
{
    return name;
}

Archer* ArcherFactory::create()
{
    return new Archer();
}

ArcherFactory::ArcherFactory()
{
    strncpy(this->name,"Archer",256);
    addDependencyCode(TECH_ARCHERY);
}

std::vector<int> ArcherFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> ArcherFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}

