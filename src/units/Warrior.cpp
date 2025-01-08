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

Warrior* WarriorFactory::create()
{
    return new Warrior();
}

WarriorFactory::WarriorFactory()
{
    strncpy(this->name,"Warrior",256);  
}

int WarriorFactory::cost(int r_id)
{
    return 40;
}

