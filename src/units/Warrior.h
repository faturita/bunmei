#ifndef WARRIOR_H
#define WARRIOR_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Warrior : public Unit, public Shippable
{
    public:
    Warrior();
    int getSubType();

};

class WarriorFactory : public BuildableFactory
{
    public:
    WarriorFactory();
    Warrior* create();
    virtual int cost(int r_id);
};

#endif   // WARRIOR_H