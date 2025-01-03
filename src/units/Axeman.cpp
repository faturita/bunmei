#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Axeman.h"

extern std::vector<Faction*> factions;

Axeman::Axeman()
{
    strcpy(name,"Axeman");
    moves = 1;
}

void Axeman::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/axeman.png", red, green, blue);
}

bool Axeman::canBuildCity()
{
    return false;
}

Axeman* AxemanFactory::create()
{
    return new Axeman();
}

AxemanFactory::AxemanFactory()
{
    strncpy(this->name,"Axeman",256);  
}

int AxemanFactory::cost(int r_id)
{
    return 40;
}

