#ifndef SETTLER_H
#define SETTLER_H

#include <iostream>
#include "Unit.h"


class Settler : public Unit
{
    public:
    Settler();

    void draw();
    bool canBuildCity();
};

#endif   // SETTLER_H


        