#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Warrior.h"

extern std::vector<Faction*> factions;

Warrior::Warrior()
{
    strcpy(name,"Warrior");
    moves = 2;
}

void Warrior::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/warrior.png", red, green, blue);
}

bool Warrior::canBuildCity()
{
    return false;
}