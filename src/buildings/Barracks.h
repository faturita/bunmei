#ifndef BARRACKS_H
#define BARRACKS_H

#include "Building.h"

class Barracks : public Building
{
    public:
    Barracks();
};

class BarracksFactory : public BuildableFactory
{
    public:
    BarracksFactory();
    virtual Buildable* create();
    virtual int cost(int r_id);
};


#endif   //BARRACKS_H