#ifndef SCOUT_H
#define SCOUT_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Scout : public Unit, public Shippable
{
    public:
    Scout();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class ScoutFactory : public BuildableFactory
{
    public:
    ScoutFactory();
    Scout* create();
    virtual int cost(int r_id);
};

#endif   // SCOUT_H