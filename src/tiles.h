#ifndef TILES_H
#define TILES_H

#include <queue>
#include <unordered_map>

#include "resources.h"
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
    LAKE = 0xe0,        
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
// improvement type with no entry is not resource-gated for BUILDING (e.g. road/irrigation/
// railroad). MINE has an entry too, but only used for commodity PRODUCTION gating
// (getRequiredImprovement) -- building a mine itself stays unrestricted, nothing checks
// this table for BuildMineOrder.
typedef std::unordered_map<int, std::vector<int>> ImprovementResources;

// Biomas an improvement type must NOT be built on (a deny-list, unlike ImprovementResources'
// allow-list), indexed by improvement type (IMPROVEMENT_TYPES, improvements.h). An
// improvement type with no entry has no bioma restriction. Matched against the tile's BASE
// bioma (bioma & 0xf0), same masking as ImprovementEffort/getImprovementEffort, so terrain
// directional-blend variants (e.g. arctic_w) still match their base bioma.
typedef std::unordered_map<int, std::vector<int>> ImprovementBiomaRestrictions;

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
    CATTLE      = 0x10a,
    OASIS       = 0x10b,
    OIL         = 0x10c,
    SEAL        = 0x10d,
    GEOSHIELD   = 0x10e,

    SILVER      = 0x10f,
    WHALES      = 0x110,
    ELEPHANTS   = 0x111,
    SILK        = 0x112,
    GRAPES      = 0x113,
    SPICES      = 0x114,
    GUNPOWDER   = 0x115,
    SUGAR       = 0x116,
    TOBACCO     = 0x117,
    COTTON      = 0x118,
    URANIUM     = 0x119,
    LITIUM      = 0x11a,
    ALUMINIUM   = 0x11b,
    HELIUM_3    = 0x11c
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
void initImprovementBiomaRestrictions(ImprovementBiomaRestrictions &restrictions);
bool tileBiomaAllowsImprovement(ImprovementBiomaRestrictions &restrictions, int improvementtype, int bioma);
// The improvement type (IMPROVEMENT_TYPES) a special resource needs before its commodity is
// produced, or 0 if it is produced unconditionally (e.g. Whales/Horse/Elephants/Silk/Spices).
int getRequiredImprovement(ImprovementResources &improvementresources, int resource);
void initCommodities(std::unordered_map<int, int> &commodityxresource);

#endif // TILES_H
