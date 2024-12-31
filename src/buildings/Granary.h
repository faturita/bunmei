#ifndef GRANARY_H
#define GRANARY_H

#include "Building.h"

class Granary : public Building
{
    public:
    Granary();
};

class GranaryFactory : public BuildableFactory
{
    public:
    GranaryFactory();
    virtual Buildable* create();
    virtual int cost(int r_id);
};


#endif   //GRANARY_H