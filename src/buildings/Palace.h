#ifndef PALACE_H
#define PALACE_H

#include "Building.h"

class Palace : public Building
{
    public:
    Palace();
};

class PalaceFactory : public BuildableFactory
{
    public:
    Palace* create();
    virtual int cost(int r_id);
};

#endif   //PALACE_H