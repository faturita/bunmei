#include "Barracks.h"

Barracks::Barracks()
{
    strcpy(name,"Barracks");
    strncpy(this->assetname,"assets/assets/city/barracks.png",256);
}

int Barracks::getSubType()
{
    return BUILDING_BARRACKS;
}

// --------------------------------------------------------

BarracksFactory::BarracksFactory()
{
    strncpy(this->name,"Barracks",256);
}

Buildable* BarracksFactory::create()
{
    Barracks* b = new Barracks();
    return b;
}

int BarracksFactory::cost(int r_id)
{
    return 50;
}

