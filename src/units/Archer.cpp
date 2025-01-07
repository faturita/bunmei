#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Archer.h"

extern std::vector<Faction*> factions;

Archer::Archer()
{
    strcpy(name,"Archer");
    strcpy(assetname,"assets/assets/units/archer.png");
    moves = 1;
    dw = 2;
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

