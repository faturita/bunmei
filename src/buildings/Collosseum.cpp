#include "Collosseum.h"

Collosseum::Collosseum()
{
    strcpy(name,"Collosseum");
    strncpy(this->assetname,"assets/assets/city/collosseum.png",256);
}

int Collosseum::getSubType()
{
    return BUILDING_COLLOSSEUM;
}

// --------------------------------------------------------

CollosseumFactory::CollosseumFactory()
{
    strncpy(this->name,"Collosseum",256);
}

Buildable* CollosseumFactory::create()
{
    Collosseum* b = new Collosseum();
    return b;
}

int CollosseumFactory::cost(int r_id)
{
    return 50;
}

