#ifndef UNIT_H
#define UNIT_H

#include <iostream>
#include <string.h>
#include <unordered_map>
#include <vector>
#include "../buildable.h"
#include "../coordinate.h"

enum MOVEMENT_TYPE
{
    LANDTYPE,
    OCEANTYPE,
    AIRTYPE
};


enum UNIT_SUBTYPE
{
    UNIT_ARCHER = 0,
    UNIT_WARRIOR = 1,
    UNIT_SPEARMAN = 2,
    UNIT_SWORDMAN = 3,
    UNIT_AXEMAN = 4,
    UNIT_HORSEMAN = 5,
    UNIT_HORSEARCHER = 6,
    UNIT_SETTLER = 7,
    UNIT_WORKER = 8,
    UNIT_TRIREME = 9,
    UNIT_GALLEY = 10,
    UNIT_CHARIOT = 11,
    UNIT_SCOUT = 12,
    UNIT_WARELEPHANT = 13,
    UNIT_PRETORIAN = 14,
    UNIT_SPY = 15
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

        bool bRoading = false;
        bool bMining = false;
        bool bIrrigating = false;
        bool bRailroading = false;

        int reqEffort;

        bool markedForDeletion = false;

        bool bDestroy = false;
        bool goBack = false;

        // A move onto a tile that costs more than the available movement is left pending:
        // the unit stays, availablemoves goes negative (movement debt) and the move completes
        // at the turn refresh (endOfYear) when availablemoves recovers to >= 0.
        bool haspendingmove = false;
        coordinate pendingmove = coordinate(0,0);

    public:
    Unit();

    coordinate target = coordinate(0,0);
    int longitude; // from the prime meridian, east is positive, west is negative.  The prime meridian can be moved.
    int latitude; // from the equator, north is positive, south is negative.
    int id;
    int faction;
    float availablemoves;       // Can go NEGATIVE: movement debt for expensive tiles.
    char name[256];

    int getUnitMoves();
    const char* getAssetName();
    // Overlay asset paths for the unit's current status (fortified/sentried/roading/...),
    // in the same order Unit::draw() paints them on the map.  Single source of truth so
    // any other view (e.g. the city screen's stationed-units box) shows the exact same
    // overlays without duplicating the fortified/sentried/isWorking flag checks; add new
    // statuses here and everywhere that calls this picks them up automatically.
    std::vector<const char*> getOverlayAssets();
    void virtual draw();
    bool virtual canBuildCity();
    int virtual cost(int r_id);
    BuildableType getType();
    int virtual getSubType();
    MOVEMENT_TYPE virtual getMovementType();

    void goTo(int lat, int lon);
    void resetGoTo();
    bool isAuto();
    bool arrived();

    void setPendingMove(coordinate c);
    bool hasPendingMove();
    coordinate getPendingMove();
    void clearPendingMove();

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

    void roading(int effort=1);
    bool isRoading();
    void mining(int effort=1);
    bool isMining();
    void irrigating(int effort=1);
    bool isIrrigating();
    void railroading(int effort=1);
    bool isRailroading();
    bool isWorking();
    void completed();
    void work();
    bool workCompleted();

    bool isMarkedForDeletion();
    bool isDying();
    void markForDeletion();
    void destroy();
    void goBackOnCompletion();

};

#endif