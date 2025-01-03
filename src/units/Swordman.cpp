#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Swordman.h"

extern std::vector<Faction*> factions;

Swordman::Swordman()
{
    strcpy(name,"Swordman");
    moves = 1;
}

void Swordman::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/swordman.png", red, green, blue);
}

bool Swordman::canBuildCity()
{
    return false;
}

Swordman* SwordmanFactory::create()
{
    return new Swordman();
}

SwordmanFactory::SwordmanFactory()
{
    strncpy(this->name,"Swordman",256);  
}

int SwordmanFactory::cost(int r_id)
{
    return 40;
}

