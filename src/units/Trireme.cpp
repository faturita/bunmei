#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Trireme.h"

extern std::vector<Faction*> factions;

Trireme::Trireme()
{
    strcpy(name,"Trireme");
    moves = 4;
    aw = 2;
}

void Trireme::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,15,"assets/assets/units/trireme.png", red, green, blue);
}

bool Trireme::canBuildCity()
{
    return false;
}

MOVEMENT_TYPE Trireme::getMovementType()
{
    return OCEANTYPE;
}

// ----------------------------

Trireme* TriremeFactory::create()
{
    return new Trireme();
}

TriremeFactory::TriremeFactory()
{
    strncpy(this->name,"Trireme",256);  
}

int TriremeFactory::cost(int r_id)
{
    return 40;
}