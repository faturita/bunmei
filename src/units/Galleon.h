#ifndef GALLEON_H
#define GALLEON_H

#include <iostream>
#include "Transport.h"
#include "Unit.h"


class Galleon : public Unit, public Transport
{
    protected:
        const int cargo = 6;

        std::unordered_map<int, Shippable*> passengers;

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