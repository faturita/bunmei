#ifndef WARRIOR_H
#define WARRIOR_H

#include <iostream>
#include "Unit.h"


class Warrior : public Unit
{
    public:
    Warrior();

    void draw();
    bool canBuildCity();
};

#endif   // WARRIOR_H