#ifndef CHARIOT_H
#define CHARIOT_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Chariot : public Unit, public Shippable
{
    public:
    Chariot();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class ChariotFactory : public BuildableFactory
{
    public:
    ChariotFactory();
    Chariot* create();
    virtual int cost(int r_id);
};

#endif   // CHARIOT_H