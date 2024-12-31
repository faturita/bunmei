#include "Palace.h"

Palace::Palace()
{
    strcpy(name,"Palace");
    strncpy(this->assetname,"assets/assets/city/palace.png",256);
}

// --------------------------------------------------------
Buildable* PalaceFactory::create()
{
    Palace* p = new Palace();
    return p;
}

PalaceFactory::PalaceFactory()
{
    strncpy(this->name,"Palace",256);
}

int PalaceFactory::cost(int r_id)
{
    return 10000;
}

