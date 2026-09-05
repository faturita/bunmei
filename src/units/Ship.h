#ifndef SHIP_H
#define SHIP_H

#include <iostream>
#include "Transport.h"
#include "Unit.h"


class Ship : public Unit, public Transport
{
    public:

        MOVEMENT_TYPE virtual getMovementType() = 0;

        virtual bool board(Shippable* passenger) = 0;
        virtual Shippable* unboard() = 0;
        virtual int manifest() = 0;
        virtual int capacity() = 0;
        virtual Shippable* findCargo(int id) = 0;
        virtual std::vector<Shippable*> getCargo() = 0;
        virtual Unit* unboardUnit() = 0;
        virtual bool removeCargo(int id) = 0;

        virtual void update(int lat, int lon) = 0;
};


#endif   // SHIP_H