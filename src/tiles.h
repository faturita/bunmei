#ifndef TILES_H
#define TILES_H

#include <queue>
#include <unordered_map>

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

enum SPECIALRESOURCES
{
    MARBLE = 0x100,
    COAL = 0x101,
    IRON = 0x102,
    COPPER = 0x103,
    GOLD = 0x104,
    DOE = 0x105,
    FISH = 0x106,
    GAME = 0x107,
    GEMS = 0x108,
    HORSE = 0x109,
    OASIS = 0x10a,
    OIL = 0x10b,
    SEAL = 0x10c,
    GEOSHIELD = 0x10d
};

typedef std::unordered_map<int, std::string> Tiles;
typedef std::unordered_map<int, std::vector<int>> Commodities;

void initTiles(std::unordered_map<int, std::string> &tiles);
void initCommodities(std::unordered_map<int, std::vector<int>> &commodities);
void initNaming(std::unordered_map<int,std::queue<std::string>> &citynames);

#endif // TILES_H
