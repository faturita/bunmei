#ifndef SWORDMAN_H
#define SWORDMAN_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Swordman : public Unit, public Shippable
{
    public:
    Swordman();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class SwordmanFactory : public BuildableFactory
{
    public:
    SwordmanFactory();
    Swordman* create();
    virtual int cost(int r_id);
};

#endif   // SWORDMAN_H