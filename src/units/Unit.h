#ifndef UNIT_H
#define UNIT_H

#include <iostream>
#include "../buildable.h"
#include "../coordinate.h"

enum MOVEMENT_TYPE
{
    LANDTYPE,
    OCEANTYPE,
    AIRTYPE
};

class Unit : public Buildable
{
    protected:
        int moves;

        bool autoMode = false;

        float e[5] = {1.0,1.0,1.0,1.0,1.0};
        float aw = 1.0;
        float dw = 1.0;
        float utw = 1.0;

        int oldlatitude;
        int oldlongitude;

        float completion=1;

        char assetname[256];

        bool fortified = false;
        bool sentried = false;
        
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
    void resetGoTo();
    bool isAuto();
    bool arrived();

    void virtual update(int newlat, int newlon);

    coordinate getCoordinate();

    float getAttack();
    float getDefense();

    bool movementCompleted();

    void fortify();
    bool isFortified();
    void packUp();

    void sentry();
    bool isSentry();
    void wakeUp();

};

#endif