#ifndef BUILDING_H
#define BUILDING_H

#include <iostream>
#include "../buildable.h"

class Building : public Buildable
{
    public:
    int faction;
    char name[256];
    void setName(const char* name);
    int virtual cost(int r_id);
    BuildableType getType();
};

#endif   //BUILDING_H