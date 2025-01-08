#ifndef HORSEARCHER_H
#define HORSEARCHER_H

#include <iostream>
#include "Unit.h"


class Horsearcher : public Unit
{
    public:
    Horsearcher();
};

class HorsearcherFactory : public BuildableFactory
{
    public:
    HorsearcherFactory();
    Horsearcher* create();
    virtual int cost(int r_id);
};

#endif   // HORSEARCHER_H