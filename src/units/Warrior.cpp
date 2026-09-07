#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Warrior.h"

extern std::vector<Faction*> factions;

Warrior::Warrior()
{
    strcpy(name,"Warrior");
    strcpy(assetname,"assets/assets/units/warrior.png");
    moves = 2;
}

int Warrior::getSubType()
{
    return UNIT_WARRIOR;
}

int Warrior::getId()
{
    return id;
}

const char* Warrior::getName()
{
    return name;
}

Warrior* WarriorFactory::create()
{
    return new Warrior();
}

WarriorFactory::WarriorFactory()
{
    strncpy(this->name,"Warrior",256);  
}

std::vector<int> WarriorFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> WarriorFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}


