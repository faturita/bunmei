#ifndef GALLEON_H
#define GALLEON_H

#include <iostream>
#include "Transport.h"
#include "Unit.h"
#include "Ship.h"


class Galleon : public Ship
{
    protected:
        const int cargo = 6;

        // Slot order (index i = cargo slot i, what the city/commerce UI shows) -- NOT keyed
        // by resource id, so two separate stacks of the SAME resource (e.g. two loads of
        // elephants once the first hits its 100-unit cap) each get their own slot instead of
        // colliding on one map entry.
        std::vector<Shippable*> passengers;

    public:
        Galleon();

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

class GalleonFactory : public BuildableFactory
{
    public:
        GalleonFactory();
        Galleon* create();
        virtual int cost(int r_id);
};

#endif   // GALLEON_H