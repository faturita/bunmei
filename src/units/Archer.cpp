#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Archer.h"

extern std::vector<Faction*> factions;

Archer::Archer()
{
    strcpy(name,"Archer");
    moves = 1;
}

void Archer::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/archer.png", red, green, blue);
}

bool Archer::canBuildCity()
{
    return false;
}

Archer* ArcherFactory::create()
{
    return new Archer();
}

ArcherFactory::ArcherFactory()
{
    strncpy(this->name,"Archer",256);  
}

int ArcherFactory::cost(int r_id)
{
    return 40;
}

