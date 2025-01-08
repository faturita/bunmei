#ifndef TRIREME_H
#define TRIREME_H

#include <iostream>
#include "Unit.h"


class Trireme : public Unit
{
    protected:
        const int cargo = 2;

        std::unordered_map<int, Unit*> passengers;

    public:
        Trireme();
        
        MOVEMENT_TYPE virtual getMovementType();

        virtual bool board(Unit* passenger);
        virtual Unit* unboard();
        virtual int manifest();

        virtual void update(int lat, int lon);
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