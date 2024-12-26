#ifndef UNIT_H
#define UNIT_H

#include <iostream>


class Unit
{
    protected:
        int moves;

    public:
    Unit();
    int longitude; // from the prime meridian, east is positive, west is negative.  The prime meridian can be moved.
    int latitude; // from the equator, north is positive, south is negative.
    int id;
    int faction;
    int availablemoves;
    char name[256];

    int getUnitMoves();
    void virtual draw();

};

#endif