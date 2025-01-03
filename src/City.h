#ifndef CITY_H
#define CITY_H

#include <unordered_map>
#include <queue>
#include <iostream>
#include "coordinate.h"
#include "buildings/Building.h"

class City
{
protected:
    //std::unordered_map<coordinate, int> tiles;

    bool isCapital;
    bool isDefended=false;

public:
    std::queue<BuildableFactory*> productionQueue;              // List of things that are currently being built within THIS particular city.
    std::vector<BuildableFactory*> buildable;                   // List of things that can be built within THIS particular city.
    std::vector<Building*> buildings;                           // List of Buildings that are already BUILT in this particular city.

    std::vector<int> resources;                                 // @FIXME Requires configuration according to the global resources.

    City(int faction, int id, int latitude, int longitude);
    int latitude;
    int longitude;
    int faction;
    int id;
    int pop;
    char name[256];

    int shields;
    int food;

    void setName(const char* name);
    void virtual draw();

    bool workingOn(int lat, int lon);
    void assignWorkingTile();
    void assignWorkingTile(coordinate c);
    void deAssigntWorkingTile();
    bool isCapitalCity();
    void setCapitalCity();
    int getProductionRate(int r_id);
    int getConsumptionRate(int r_id);
    int numberOfWorkingTiles();
    bool occupied(int lat, int lon);
    void setDefense();
    void noDefense();
    coordinate getCoordinate();


};

#endif // CITY_H