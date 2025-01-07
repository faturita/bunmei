#ifndef SPEARMAN_H
#define SPEARMAN_H

#include <iostream>
#include "Unit.h"


class Spearman : public Unit
{
    public:
    Spearman();

    bool canBuildCity();
};

class SpearmanFactory : public BuildableFactory
{
    public:
    SpearmanFactory();
    Spearman* create();
    virtual int cost(int r_id);
};

#endif   // SPEARMAN_H