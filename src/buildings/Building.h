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
    BUILDING_COLLOSSEUM = 3
};

class Building : public Buildable
{
    public:
    Building();
    int faction;
    char name[256];
    char assetname[256];
    void setName(const char* name);
    BuildableType getType();
    int virtual getSubType();
};

#endif   //BUILDING_H