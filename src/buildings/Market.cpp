#include "Market.h"

Market::Market()
{
    strcpy(name,"Market");
    strncpy(this->assetname,"assets/assets/city/market.png",256);
}

int Market::getSubType()
{
    return BUILDING_MARKET;
}

// --------------------------------------------------------
MarketFactory::MarketFactory()
{
    strncpy(this->name,"Market",256);
}

Buildable* MarketFactory::create()
{
    Market* b = new Market();
    return b;
}

int MarketFactory::cost(int r_id)
{
    return 80;
}

