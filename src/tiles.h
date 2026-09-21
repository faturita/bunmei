#ifndef TILES_H
#define TILES_H

#include <array>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

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

// ---------------------------------------------------------------------------------------
// Tile production.
//
// Every tile carries its OWN base production rates (mapcell::resource_production_rate), so a
// single tile can be given a yield nothing else on the map has -- a scenario, an event, a
// one-off. assignProductionRates() (engine.h) populates all of them from the tables below,
// and it runs BOTH when a world is generated from scratch and when one is loaded, so the
// stored map never has to carry a yield at all.
//
// That last part is the fix to a real defect: mapio.cpp used to SAVE the rate vector, writing
// the IMPROVEMENT-ADJUSTED value (getResourceProductionRate applies the factor) where the
// BASE belongs. Anything that loaded a map without re-assigning afterwards read those
// inflated numbers as the base and applied the improvement bonus a second time -- and saving
// again compounded it. initMap() did re-assign, which is why the live game got away with it;
// the file was simply storing a derived value it had no business storing. Now only what a
// tile IS gets saved -- terrain/bioma, special resource, improvements -- and the rates are
// rebuilt from these tables on both paths.

// Context keys for the two tables: real biomas (BIOMAS above) are all >= 0x20, so these
// negative sentinels never collide with one.
#define OCEAN_CONTEXT   -2   // any OCEAN-coded tile, regardless of bioma
#define ANY_LAND_BIOMA  -1   // any LAND-coded tile, regardless of bioma (e.g. GEMS, GOLD)

// How a special resource (SPECIALRESOURCES) nudges the rates on top of the base, for a given
// context. Only the listed RESOURCE_TYPES indices are overridden; the rest keep the base.
struct ResourceRateOverride
{
    int context;                              // OCEAN_CONTEXT, a BIOMAS constant, or ANY_LAND_BIOMA
    int resource;                             // a SPECIALRESOURCES constant
    std::vector<std::pair<int,int>> rates;    // (RESOURCE_TYPES index, value) pairs
};

// The whole production model, in one table initialized once at startup (initProductionRates)
// by the game, the simulator and the testcase harness alike -- it used to be duplicated in
// gamekernel.cpp and simulate.cpp, and the two copies had silently drifted apart.
struct ProductionRates
{
    // Fills itself from initProductionRates(). The tables have to be valid before the first
    // tile yield is read, and that happens in three different builds (the game, the headless
    // simulator, and every testcase's init()) -- so rather than rely on each of them
    // remembering a call, the default state IS the initialized state. initProductionRates()
    // stays public and idempotent: the game still calls it explicitly at startup alongside
    // initTiles() and friends, and a scenario is free to re-fill the table.
    ProductionRates();

    // Base rate per terrain/bioma, before any special resource. Array order matches
    // RESOURCE_TYPES (resources.h): FOOD, SHIELDS, TRADE, COINS, SCIENCE, CULTURE. Bioma
    // variants share their base bioma's rate (looked up by bioma & 0xf0, same convention as
    // MovementCost). A LAND bioma with no entry here (undecorated land, jungle, tundra, ...)
    // falls back to `defaultland`.
    std::unordered_map<int, std::array<int,6>> base;
    std::array<int,6>                          defaultland;
    std::vector<ResourceRateOverride>          overrides;

    // [RESOURCE_TYPES index][improvement][factor, additive], improvements in IMPROVEMENT_TYPES
    // bit order: Irrigation, Mine, Road, Railroad. Applied on top of the base+override rate
    // for every improvement present on the tile. This lived as a 192-byte member of EVERY
    // mapcell, identical in all of them.
    float improvement[6][4][2];
};

extern ProductionRates productionrates;

void initProductionRates(ProductionRates &rates);

// The BASE rates (one per RESOURCE_TYPES index) the tables give a tile that is `code`/`bioma`
// and carries `resource`: the bioma's row, with any special-resource override applied on top.
// This is what assignProductionRates() writes into each cell. Improvements are NOT included
// -- they are a live multiplier applied by mapcell::getResourceProductionRate(), so building
// a road changes the yield without anything having to be re-assigned.
std::array<int,6> tileBaseProductionRates(const ProductionRates &rates, int code, int bioma, int resource);

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

// Seeds `prices` (tiles.cpp) with a unit price of 1 for every commodity and mfg good.
void initPrices(std::unordered_map<int, int> &prices);

#endif // TILES_H
