#ifndef SPEARMAN_H
#define SPEARMAN_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Spearman : public Unit, public Shippable
{
    public:
    Spearman();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class SpearmanFactory : public BuildableFactory
{
    public:
    SpearmanFactory();
    Spearman* create();
    virtual int cost(int r_id);
};

#endif   // SPEARMAN_H