#ifndef COLLOSSEUM_H
#define COLLOSSEUM_H

#include "Building.h"

class Collosseum : public Building
{
    public:
    Collosseum();
};

// --------------------------------------------------------
class CollosseumFactory : public BuildableFactory
{
    public:
    CollosseumFactory();
    virtual Buildable* create();
    virtual int cost(int r_id);
};


#endif   //COLLOSSEUM_H