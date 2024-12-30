#include "Barracks.h"

Barracks::Barracks()
{
    strcpy(name,"Barracks");
}

Barracks* BarracksFactory::create()
{
    Barracks* b = new Barracks();
}

int BarracksFactory::cost(int r_id)
{
    return 50;
}

