#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Horseman.h"

extern std::vector<Faction*> factions;

Horseman::Horseman()
{
    strcpy(name,"Horseman");
    moves = 4;
}

void Horseman::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/horseman.png", red, green, blue);
}

bool Horseman::canBuildCity()
{
    return false;
}

// ----------------------------

Horseman* HorsemanFactory::create()
{
    return new Horseman();
}

HorsemanFactory::HorsemanFactory()
{
    strncpy(this->name,"Horseman",256);  
}

int HorsemanFactory::cost(int r_id)
{
    return 40;
}