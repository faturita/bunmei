#ifndef TRIREME_H
#define TRIREME_H

#include <iostream>
#include "Transport.h"
#include "Unit.h"


class Trireme : public Unit, Transport
{
    protected:
        const int cargo = 2;

        std::unordered_map<int, Shippable*> passengers;

    public:
        Trireme();
        
        MOVEMENT_TYPE virtual getMovementType();

        virtual bool board(Shippable* passenger);
        virtual Shippable* unboard();
        virtual int manifest();

        virtual void update(int lat, int lon);

        int getSubType();
};

// -----------------------------------------------

class TriremeFactory : public BuildableFactory
{
    public:
        TriremeFactory();
        Trireme* create();
        virtual int cost(int r_id);
};

#endif   // TRIREME_H