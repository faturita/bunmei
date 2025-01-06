#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Spearman.h"

extern std::vector<Faction*> factions;

Spearman::Spearman()
{
    strcpy(name,"Spearman");
    moves = 1;
    dw = 3;
}

void Spearman::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/spearman.png", red, green, blue);
}

bool Spearman::canBuildCity()
{
    return false;
}

Spearman* SpearmanFactory::create()
{
    return new Spearman();
}

SpearmanFactory::SpearmanFactory()
{
    strncpy(this->name,"Spearman",256);  
}

int SpearmanFactory::cost(int r_id)
{
    return 40;
}

