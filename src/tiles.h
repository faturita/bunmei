#ifndef TILES_H
#define TILES_H

#include <queue>
#include <unordered_map>

#include "improvements.h"

enum TERRAIN
{
    OCEAN = 0,
    LAND = 1
};

enum BIOMAS
{
    ARCTIC = 0x20,
    DESERT = 0x30,
    FOREST = 0x40,
    GRASSLAND = 0x50,
    HILLS = 0x60,
    JUNGLE = 0x70,
    MOUNTAINS = 0x80,
    PLAINS = 0x90,
    RIVER = 0xa0,
    SWAMP = 0xb0,
    TUNDRA = 0xc0,
    OCEANBIOMA = 0xd0,
    LANDBIOMA = 0x01,
    RIVER_MOUTH_W = 0x02,
    RIVER_MOUTH_S = 0x03,
    RIVER_MOUTH_E = 0x04,
    RIVER_MOUTH_N = 0x05
};

typedef std::unordered_map<int, float> MovementCost;

// Moving along a road or a railroad (both tiles connected by it) overrides the
// bioma movement cost with these values.
#define ROAD_MOVEMENT_COST      (1.0f/3.0f)
#define RAILROAD_MOVEMENT_COST  (1.0f/9.0f)

// Effort (in worker turns) required to complete an improvement, indexed by
// [improvement type (IMPROVEMENT_TYPES, improvements.h)][base bioma] (bioma & 0xf0, same
// masking as MovementCost).
typedef std::unordered_map<int, std::unordered_map<int, int>> ImprovementEffort;

// Special resources (SPECIALRESOURCES, below) a tile must have for an improvement to be
// built there, indexed by improvement type (IMPROVEMENT_TYPES, improvements.h). An
// improvement type with no entry is not resource-gated (e.g. road/irrigation/mine/railroad).
typedef std::unordered_map<int, std::vector<int>> ImprovementResources;

enum SPECIALRESOURCES
{
    MARBLE      = 0x100,
    CARBON      = 0x101,   
    IRON        = 0x102,
    COPPER      = 0x103,
    GOLD        = 0x104,
    DOE         = 0x105,
    FISH        = 0x106,
    GAME        = 0x107,
    GEMS        = 0x108,
    HORSE       = 0x109,
    OASIS       = 0x10a,
    OIL         = 0x10b,
    SEAL        = 0x10c,
    GEOSHIELD   = 0x10d,

    SILVER      = 0x10e,
    WHALES      = 0x10f,
    ELEPHANTS   = 0x110,
    SILK        = 0x111,
    GRAPES      = 0x112,
    SPICES      = 0x113,
    SUGAR       = 0x114,
    TOBACCO     = 0x115,
    COTTON      = 0x116,
    URANIUM     = 0x118,
    LITIUM      = 0x119,
    ALUMINIUM   = 0x11a,
    HELIUM_3    = 0x11b   
};


enum COMMODITIES
{
    copper     = 0x201,
    iron       = 0x202,
    silver     = 0x203,
    marble     = 0x204,
    furs       = 0x205,
    traan      = 0x206,
    gems       = 0x207,
    horses     = 0x208,
    elephants  = 0x209,
    silk       = 0x20a,
    wine       = 0x20b,
    spices     = 0x20c,
    sugar      = 0x20d,
    tobacco    = 0x20e,
    cotton     = 0x20f,
    carbon     = 0x210,
    uranium    = 0x211,
    oil        = 0x212,
    litium     = 0x213,
    aluminium  = 0x214,
    helium_3   = 0x215
};

typedef std::unordered_map<int, std::string> Tiles;
typedef std::unordered_map<int, std::vector<int>> Commodities;

void initTiles(std::unordered_map<int, std::string> &tiles);
void initResources(std::unordered_map<int, std::vector<int>> &resourcexbioma);
void initNaming(std::unordered_map<int,std::queue<std::string>> &citynames);
void initMovementCosts(MovementCost &movementcosts);
void initImprovementEffort(ImprovementEffort &improvementeffort);
int getImprovementEffort(ImprovementEffort &improvementeffort, int improvementtype, int bioma);
void initImprovementResources(ImprovementResources &improvementresources);
bool tileHasRequiredResource(ImprovementResources &improvementresources, int improvementtype, int resource);
void initCommodities(std::unordered_map<int, int> &commodityxresource);

#endif // TILES_H
