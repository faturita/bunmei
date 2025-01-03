#ifndef UNIT_H
#define UNIT_H

#include <iostream>
#include "../buildable.h"
#include "../coordinate.h"

enum MOVEMENT_TYPE
{
    LANDTYPE,
    SEATYPE,
    AIRTYPE
};

class Unit : public Buildable
{
    protected:
        int moves;

        bool autoMode = false;
    public:
    Unit();

    coordinate target = coordinate(0,0);
    int longitude; // from the prime meridian, east is positive, west is negative.  The prime meridian can be moved.
    int latitude; // from the equator, north is positive, south is negative.
    int id;
    int faction;
    int availablemoves;
    char name[256];

    int getUnitMoves();
    void virtual draw();
    bool virtual canBuildCity();
    int virtual cost(int r_id);
    BuildableType getType();
    MOVEMENT_TYPE virtual getMovementType();

    void goTo(int lat, int lon);
    bool isAuto();
    bool arrived();

    coordinate getCoordinate();

};

#endif