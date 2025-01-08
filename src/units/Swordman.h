#ifndef SWORDMAN_H
#define SWORDMAN_H

#include <iostream>
#include "Unit.h"


class Swordman : public Unit
{
    public:
    Swordman();
};

class SwordmanFactory : public BuildableFactory
{
    public:
    SwordmanFactory();
    Swordman* create();
    virtual int cost(int r_id);
};

#endif   // SWORDMAN_H