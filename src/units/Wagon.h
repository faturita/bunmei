#ifndef WAGON_H
#define WAGON_H

#include <iostream>
#include "Unit.h"


class Wagon : public Unit
{
    protected:
        const int cargo = 2;

        std::unordered_map<int, Unit*> passengers;

    public:
        Wagon();
        
        MOVEMENT_TYPE virtual getMovementType();

        virtual bool board(Unit* passenger);
        virtual Unit* unboard();
        virtual int manifest();

        virtual void update(int lat, int lon);

        int getSubType();
};

// -----------------------------------------------

class WagonFactory : public BuildableFactory
{
    public:
        WagonFactory();
        Wagon* create();
        virtual int cost(int r_id);
};

#endif   // WAGON_H