#ifndef BUILDING_H
#define BUILDING_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstring>
#include "../buildable.h"

enum BUILDING_SUBTYPE
{
    BUILDING_PALACE = 0,
    BUILDING_BARRACKS = 1,
    BUILDING_GRANARY = 2,
    BUILDING_COLLOSSEUM = 3,
    BUILDING_MARKET = 4,
    BUILDING_TEMPLE = 5,
    BUILDING_LIBRARY = 6,
    BUILDING_WALLS = 7,
    BUILDING_HARBOR = 8,
    BUILDING_WAREHOUSE = 9,
    BUILDING_DEPOT = 10,
    BUILDING_FACTORY = 11
    
};

class Building : public Buildable
{
    protected:
        std::vector<int> perkCodes;
    public:
    Building();
    int faction;
    char name[256];
    char assetname[256];
    void setName(const char* name);
    BuildableType getType();
    int virtual getSubType();
    std::vector<int>& getPerkCodes();

    int virtual getProductionRate(int r_id);
    int virtual getConsumptionRate(int r_id);
};

#endif   //BUILDING_H