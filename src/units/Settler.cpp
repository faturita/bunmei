#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Settler.h"


extern std::vector<Faction*> factions;

Settler::Settler()
{
    strcpy(name,"Settler");
    moves = 2;
}

void Settler::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/settlers.png", red, green, blue);
}

bool Settler::canBuildCity()
{
    return true;
}