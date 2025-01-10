#include <vector>
#include "resources.h"

void initResources(std::vector<Resource*> &resources)
{
    resources.push_back(new Resource(FOOD,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(SHIELDS,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(TRADE,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(COINS,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(SCIENCE,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(CULTURE,0,"assets/assets/city/culture.png","Culture"));
}