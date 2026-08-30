#ifndef HORSEARCHER_H
#define HORSEARCHER_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Horsearcher : public Unit, public Shippable
{
    public:
    Horsearcher();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class HorsearcherFactory : public BuildableFactory
{
    public:
    HorsearcherFactory();
    Horsearcher* create();
    virtual int cost(int r_id);
};

#endif   // HORSEARCHER_H