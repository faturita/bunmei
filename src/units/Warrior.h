#ifndef WARRIOR_H
#define WARRIOR_H

#include <iostream>
#include "Unit.h"


class Warrior : public Unit
{
    public:
    Warrior();

};

class WarriorFactory : public BuildableFactory
{
    public:
    WarriorFactory();
    Warrior* create();
    virtual int cost(int r_id);
};

#endif   // WARRIOR_H