#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Horsearcher.h"

extern std::vector<Faction*> factions;

Horsearcher::Horsearcher()
{
    strcpy(name,"Horsearcher");
    moves = 4;
}

void Horsearcher::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/horsearcher.png", red, green, blue);
}

bool Horsearcher::canBuildCity()
{
    return false;
}

// ----------------------------

Horsearcher* HorsearcherFactory::create()
{
    return new Horsearcher();
}

HorsearcherFactory::HorsearcherFactory()
{
    strncpy(this->name,"Horsearcher",256);  
}

int HorsearcherFactory::cost(int r_id)
{
    return 40;
}