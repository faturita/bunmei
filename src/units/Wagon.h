#ifndef WAGON_H
#define WAGON_H

#include <iostream>
#include "Transport.h"
#include "Unit.h"


class Wagon : public Unit, public Transport
{
    protected:
        const int cargo = 2;

        std::unordered_map<int, Shippable*> passengers;

    public:
        Wagon();

        MOVEMENT_TYPE virtual getMovementType();

        virtual bool board(Shippable* passenger);
        virtual Shippable* unboard();
        virtual int manifest();
        virtual int capacity();
        virtual Shippable* findCargo(int id);
        virtual std::vector<Shippable*> getCargo();
        virtual Unit* unboardUnit();
        virtual bool removeCargo(int id);

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