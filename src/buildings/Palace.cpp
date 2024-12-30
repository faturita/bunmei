#include "Palace.h"

Palace::Palace()
{
    strcpy(name,"Palace");
}

Palace* PalaceFactory::create()
{
    Palace* p = new Palace();
    return p;
}

int PalaceFactory::cost(int r_id)
{
    return 1000;
}

