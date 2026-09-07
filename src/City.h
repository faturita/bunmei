#ifndef CITY_H
#define CITY_H

#include <unordered_map>
#include <queue>
#include <iostream>
#include <string.h>
#include "coordinate.h"
#include "buildings/Building.h"
#include "mapmodel.h"
#include "Faction.h"

int getPopulationThresshold(int pop);

class City
{
protected:
    //std::unordered_map<coordinate, int> tiles;

    bool isCapital;
    bool isDefended=false;

    Map *map;

public:

    std::queue<BuildableFactory*> productionQueue;              // List of things that are currently being built within THIS particular city.
    std::vector<BuildableFactory*> buildable;                   // List of things that can be built within THIS particular city.
    std::vector<Building*> buildings;                           // List of Buildings that are already BUILT in this particular city.

    // One stockpile for ALL three resource classes -- core resources (CORE_RESOURCES ids,
    // worked from tiles), commodities (COMMODITIES ids, gathered from special resources in
    // range) and manufactured goods (MFGOODS ids, produced by buildings). The id ranges are
    // disjoint (0x0x / 0x2xx / 0x3xx) so callers still iterate ALL_CORE_RESOURCES /
    // ALL_COMMODITIES / ALL_MFG_GOODS / ALL_COMMODITIES_AND_MFGGOODS to pick a class.
    std::unordered_map<int, int> resources;


    City(Map *map, int faction, int id, int latitude, int longitude);
    int latitude;
    int longitude;
    int faction;
    int id;
    int pop;
    char name[256];
    int foundedyear;

    int shields;
    int food;

    void setName(const char* name);
    void virtual draw();

    bool workingOn(int lat, int lon);
    void assignWorkingTile();
    void assignWorkingTile(coordinate c);
    void deAssigntWorkingTile();
    void reAssignWorkingTiles(int new_f_id);
    bool isCapitalCity();
    void setCapitalCity();
    int getProductionRate(int r_id);
    int getConsumptionRate(int r_id);
    // Total upkeep of resource r_id across every Building already built in this city
    // (sum of Building::getConsumptionRate). Kept SEPARATE from getConsumptionRate() (which
    // is pop/tile upkeep) because endOfYear deducts building upkeep via operateCityBuildings()
    // instead -- folding it into getConsumptionRate() would double-charge it.
    int getBuildingConsumptionRate(int r_id);
    int getCommodityProductionRate(int commodity_id);
    int numberOfWorkingTiles();
    bool occupied(int lat, int lon);
    void setDefense();
    void noDefense();
    bool isDefendedCity();
    coordinate getCoordinate();


};

#endif // CITY_H