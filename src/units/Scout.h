#ifndef SCOUT_H
#define SCOUT_H

#include <iostream>
#include "Unit.h"


class Scout : public Unit
{
    public:
    Scout();
    int getSubType();
};

class ScoutFactory : public BuildableFactory
{
    public:
    ScoutFactory();
    Scout* create();
    virtual int cost(int r_id);
};

#endif   // SCOUT_H