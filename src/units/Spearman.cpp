#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Spearman.h"

extern std::vector<Faction*> factions;

Spearman::Spearman()
{
    strcpy(name,"Spearman");
    strcpy(assetname,"assets/assets/units/spearman.png");
    moves = 1;
    dw = 3;
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

