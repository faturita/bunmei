#ifndef BUILDING_H
#define BUILDING_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstring>
#include "../buildable.h"

class Building : public Buildable
{
    public:
    Building();
    int faction;
    char name[256];
    char assetname[256];
    void setName(const char* name);
    BuildableType getType();
};

#endif   //BUILDING_H