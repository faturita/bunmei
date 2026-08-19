#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include "tiles.h"




void initTiles(std::unordered_map<int, std::string> &tiles)
{
    tiles[OCEAN] = "assets/assets/terrain/ocean.png";
    tiles[LAND] = "assets/assets/terrain/land.png";

    tiles[RIVER_MOUTH_W] = "assets/assets/terrain/river_mouth_w.png";
    tiles[RIVER_MOUTH_S] = "assets/assets/terrain/river_mouth_s.png";
    tiles[RIVER_MOUTH_E] = "assets/assets/terrain/river_mouth_e.png";
    tiles[RIVER_MOUTH_N] = "assets/assets/terrain/river_mouth_n.png";

    // These are the basic building block tiles for all the biomas.
    tiles[ARCTIC] = "assets/assets/terrain/arctic.png";
    tiles[DESERT] = "assets/assets/terrain/desert.png";
    tiles[FOREST] = "assets/assets/terrain/forest.png";
    tiles[GRASSLAND] = "assets/assets/terrain/grassland.png";
    tiles[HILLS] = "assets/assets/terrain/hills.png";
    tiles[JUNGLE] = "assets/assets/terrain/jungle.png";
    tiles[MOUNTAINS] = "assets/assets/terrain/mountains.png";
    tiles[PLAINS] = "assets/assets/terrain/plains.png";
    tiles[RIVER] = "assets/assets/terrain/river.png";
    tiles[SWAMP] = "assets/assets/terrain/swamp.png";
    tiles[TUNDRA] = "assets/assets/terrain/tundra.png";
    tiles[OCEANBIOMA] = "assets/assets/terrain/ocean.png";

    tiles[0x20] = "assets/assets/terrain/arctic.png";
    tiles[0x21] = "assets/assets/terrain/arctic_w.png";
    tiles[0x22] = "assets/assets/terrain/arctic_s.png";
    tiles[0x23] = "assets/assets/terrain/arctic_sw.png";
    tiles[0x24] = "assets/assets/terrain/arctic_e.png";
    tiles[0x25] = "assets/assets/terrain/arctic_ew.png";
    tiles[0x26] = "assets/assets/terrain/arctic_es.png";
    tiles[0x27] = "assets/assets/terrain/arctic_esw.png";
    tiles[0x28] = "assets/assets/terrain/arctic_n.png";
    tiles[0x29] = "assets/assets/terrain/arctic_nw.png";
    tiles[0x2a] = "assets/assets/terrain/arctic_ns.png";
    tiles[0x2b] = "assets/assets/terrain/arctic_nsw.png";
    tiles[0x2c] = "assets/assets/terrain/arctic_ne.png";
    tiles[0x2d] = "assets/assets/terrain/arctic_new.png";
    tiles[0x2e] = "assets/assets/terrain/arctic_nes.png";
    tiles[0x2f] = "assets/assets/terrain/arctic_nesw.png";

    tiles[0x30] = "assets/assets/terrain/desert.png";
    tiles[0x31] = "assets/assets/terrain/desert_w.png";
    tiles[0x32] = "assets/assets/terrain/desert_s.png";
    tiles[0x33] = "assets/assets/terrain/desert_sw.png";
    tiles[0x34] = "assets/assets/terrain/desert_e.png";
    tiles[0x35] = "assets/assets/terrain/desert_ew.png";
    tiles[0x36] = "assets/assets/terrain/desert_es.png";
    tiles[0x37] = "assets/assets/terrain/desert_esw.png";
    tiles[0x38] = "assets/assets/terrain/desert_n.png";
    tiles[0x39] = "assets/assets/terrain/desert_nw.png";
    tiles[0x3a] = "assets/assets/terrain/desert_ns.png";
    tiles[0x3b] = "assets/assets/terrain/desert_nsw.png";
    tiles[0x3c] = "assets/assets/terrain/desert_ne.png";
    tiles[0x3d] = "assets/assets/terrain/desert_new.png";
    tiles[0x3e] = "assets/assets/terrain/desert_nes.png";
    tiles[0x3f] = "assets/assets/terrain/desert_nesw.png";

    tiles[0x40] = "assets/assets/terrain/forest.png";
    tiles[0x41] = "assets/assets/terrain/forest_w.png";
    tiles[0x42] = "assets/assets/terrain/forest_s.png";
    tiles[0x43] = "assets/assets/terrain/forest_sw.png";
    tiles[0x44] = "assets/assets/terrain/forest_e.png";
    tiles[0x45] = "assets/assets/terrain/forest_ew.png";
    tiles[0x46] = "assets/assets/terrain/forest_es.png";
    tiles[0x47] = "assets/assets/terrain/forest_esw.png";
    tiles[0x48] = "assets/assets/terrain/forest_n.png";
    tiles[0x49] = "assets/assets/terrain/forest_nw.png";
    tiles[0x4a] = "assets/assets/terrain/forest_ns.png";
    tiles[0x4b] = "assets/assets/terrain/forest_nsw.png";
    tiles[0x4c] = "assets/assets/terrain/forest_ne.png";
    tiles[0x4d] = "assets/assets/terrain/forest_new.png";
    tiles[0x4e] = "assets/assets/terrain/forest_nes.png";
    tiles[0x4f] = "assets/assets/terrain/forest_nesw.png";

    tiles[0x50] = "assets/assets/terrain/grassland.png";
    tiles[0x51] = "assets/assets/terrain/grassland_w.png";
    tiles[0x52] = "assets/assets/terrain/grassland_s.png";
    tiles[0x53] = "assets/assets/terrain/grassland_sw.png";
    tiles[0x54] = "assets/assets/terrain/grassland_e.png";
    tiles[0x55] = "assets/assets/terrain/grassland_ew.png";
    tiles[0x56] = "assets/assets/terrain/grassland_es.png";
    tiles[0x57] = "assets/assets/terrain/grassland_esw.png";
    tiles[0x58] = "assets/assets/terrain/grassland_n.png";
    tiles[0x59] = "assets/assets/terrain/grassland_nw.png";
    tiles[0x5a] = "assets/assets/terrain/grassland_ns.png";
    tiles[0x5b] = "assets/assets/terrain/grassland_nsw.png";
    tiles[0x5c] = "assets/assets/terrain/grassland_ne.png";
    tiles[0x5d] = "assets/assets/terrain/grassland_new.png";
    tiles[0x5e] = "assets/assets/terrain/grassland_nes.png";
    tiles[0x5f] = "assets/assets/terrain/grassland_nesw.png";
    
    tiles[0x60] = "assets/assets/terrain/hills.png";
    tiles[0x61] = "assets/assets/terrain/hills_w.png";
    tiles[0x62] = "assets/assets/terrain/hills_s.png";
    tiles[0x63] = "assets/assets/terrain/hills_sw.png";
    tiles[0x64] = "assets/assets/terrain/hills_e.png";
    tiles[0x65] = "assets/assets/terrain/hills_ew.png";
    tiles[0x66] = "assets/assets/terrain/hills_es.png";
    tiles[0x67] = "assets/assets/terrain/hills_esw.png";
    tiles[0x68] = "assets/assets/terrain/hills_n.png";
    tiles[0x69] = "assets/assets/terrain/hills_nw.png";
    tiles[0x6a] = "assets/assets/terrain/hills_ns.png";
    tiles[0x6b] = "assets/assets/terrain/hills_nsw.png";
    tiles[0x6c] = "assets/assets/terrain/hills_ne.png";
    tiles[0x6d] = "assets/assets/terrain/hills_new.png";
    tiles[0x6e] = "assets/assets/terrain/hills_nes.png";
    tiles[0x6f] = "assets/assets/terrain/hills_nesw.png";

    tiles[0x70] = "assets/assets/terrain/jungle.png";
    tiles[0x71] = "assets/assets/terrain/jungle_w.png";
    tiles[0x72] = "assets/assets/terrain/jungle_s.png";
    tiles[0x73] = "assets/assets/terrain/jungle_sw.png";
    tiles[0x74] = "assets/assets/terrain/jungle_e.png";
    tiles[0x75] = "assets/assets/terrain/jungle_ew.png";
    tiles[0x76] = "assets/assets/terrain/jungle_es.png";
    tiles[0x77] = "assets/assets/terrain/jungle_esw.png";
    tiles[0x78] = "assets/assets/terrain/jungle_n.png";
    tiles[0x79] = "assets/assets/terrain/jungle_nw.png";
    tiles[0x7a] = "assets/assets/terrain/jungle_ns.png";
    tiles[0x7b] = "assets/assets/terrain/jungle_nsw.png";
    tiles[0x7c] = "assets/assets/terrain/jungle_ne.png";
    tiles[0x7d] = "assets/assets/terrain/jungle_new.png";
    tiles[0x7e] = "assets/assets/terrain/jungle_nes.png";
    tiles[0x7f] = "assets/assets/terrain/jungle_nesw.png";

    tiles[0x80] = "assets/assets/terrain/mountains.png";
    tiles[0x81] = "assets/assets/terrain/mountains_w.png";
    tiles[0x82] = "assets/assets/terrain/mountains_s.png";
    tiles[0x83] = "assets/assets/terrain/mountains_sw.png";
    tiles[0x84] = "assets/assets/terrain/mountains_e.png";
    tiles[0x85] = "assets/assets/terrain/mountains_ew.png";
    tiles[0x86] = "assets/assets/terrain/mountains_es.png";
    tiles[0x87] = "assets/assets/terrain/mountains_esw.png";
    tiles[0x88] = "assets/assets/terrain/mountains_n.png";
    tiles[0x89] = "assets/assets/terrain/mountains_nw.png";
    tiles[0x8a] = "assets/assets/terrain/mountains_ns.png";
    tiles[0x8b] = "assets/assets/terrain/mountains_nsw.png";
    tiles[0x8c] = "assets/assets/terrain/mountains_ne.png";
    tiles[0x8d] = "assets/assets/terrain/mountains_new.png";
    tiles[0x8e] = "assets/assets/terrain/mountains_nes.png";
    tiles[0x8f] = "assets/assets/terrain/mountains_nesw.png";

    tiles[0x90] = "assets/assets/terrain/plains.png";
    tiles[0x91] = "assets/assets/terrain/plains_w.png";
    tiles[0x92] = "assets/assets/terrain/plains_s.png";
    tiles[0x93] = "assets/assets/terrain/plains_sw.png";
    tiles[0x94] = "assets/assets/terrain/plains_e.png";
    tiles[0x95] = "assets/assets/terrain/plains_ew.png";
    tiles[0x96] = "assets/assets/terrain/plains_es.png";
    tiles[0x97] = "assets/assets/terrain/plains_esw.png";
    tiles[0x98] = "assets/assets/terrain/plains_n.png";
    tiles[0x99] = "assets/assets/terrain/plains_nw.png";
    tiles[0x9a] = "assets/assets/terrain/plains_ns.png";
    tiles[0x9b] = "assets/assets/terrain/plains_nsw.png";
    tiles[0x9c] = "assets/assets/terrain/plains_ne.png";
    tiles[0x9d] = "assets/assets/terrain/plains_new.png";
    tiles[0x9e] = "assets/assets/terrain/plains_nes.png";
    tiles[0x9f] = "assets/assets/terrain/plains_nesw.png";

    tiles[0xa0] = "assets/assets/terrain/river.png";
    tiles[0xa1] = "assets/assets/terrain/river_w.png";
    tiles[0xa2] = "assets/assets/terrain/river_s.png";
    tiles[0xa3] = "assets/assets/terrain/river_sw.png";
    tiles[0xa4] = "assets/assets/terrain/river_e.png";
    tiles[0xa5] = "assets/assets/terrain/river_ew.png";
    tiles[0xa6] = "assets/assets/terrain/river_es.png";
    tiles[0xa7] = "assets/assets/terrain/river_esw.png";
    tiles[0xa8] = "assets/assets/terrain/river_n.png";
    tiles[0xa9] = "assets/assets/terrain/river_nw.png";
    tiles[0xaa] = "assets/assets/terrain/river_ns.png";
    tiles[0xab] = "assets/assets/terrain/river_nsw.png";
    tiles[0xac] = "assets/assets/terrain/river_ne.png";
    tiles[0xad] = "assets/assets/terrain/river_new.png";
    tiles[0xae] = "assets/assets/terrain/river_nes.png";
    tiles[0xaf] = "assets/assets/terrain/river_nesw.png";

    tiles[0xb0] = "assets/assets/terrain/swamp.png";
    tiles[0xb1] = "assets/assets/terrain/swamp_w.png";
    tiles[0xb2] = "assets/assets/terrain/swamp_s.png";
    tiles[0xb3] = "assets/assets/terrain/swamp_sw.png";
    tiles[0xb4] = "assets/assets/terrain/swamp_e.png";
    tiles[0xb5] = "assets/assets/terrain/swamp_ew.png";
    tiles[0xb6] = "assets/assets/terrain/swamp_es.png";
    tiles[0xb7] = "assets/assets/terrain/swamp_esw.png";
    tiles[0xb8] = "assets/assets/terrain/swamp_n.png";
    tiles[0xb9] = "assets/assets/terrain/swamp_nw.png";
    tiles[0xba] = "assets/assets/terrain/swamp_ns.png";
    tiles[0xbb] = "assets/assets/terrain/swamp_nsw.png";
    tiles[0xbc] = "assets/assets/terrain/swamp_ne.png";
    tiles[0xbd] = "assets/assets/terrain/swamp_new.png";
    tiles[0xbe] = "assets/assets/terrain/swamp_nes.png";
    tiles[0xbf] = "assets/assets/terrain/swamp_nesw.png";

    tiles[0xc0] = "assets/assets/terrain/tundra.png";
    tiles[0xc1] = "assets/assets/terrain/tundra_w.png";
    tiles[0xc2] = "assets/assets/terrain/tundra_s.png";
    tiles[0xc3] = "assets/assets/terrain/tundra_sw.png";
    tiles[0xc4] = "assets/assets/terrain/tundra_e.png"; 
    tiles[0xc5] = "assets/assets/terrain/tundra_ew.png";
    tiles[0xc6] = "assets/assets/terrain/tundra_es.png";
    tiles[0xc7] = "assets/assets/terrain/tundra_esw.png";
    tiles[0xc8] = "assets/assets/terrain/tundra_n.png";
    tiles[0xc9] = "assets/assets/terrain/tundra_nw.png";
    tiles[0xca] = "assets/assets/terrain/tundra_ns.png";
    tiles[0xcb] = "assets/assets/terrain/tundra_nsw.png";
    tiles[0xcc] = "assets/assets/terrain/tundra_ne.png";
    tiles[0xcd] = "assets/assets/terrain/tundra_new.png";
    tiles[0xce] = "assets/assets/terrain/tundra_nes.png";
    tiles[0xcf] = "assets/assets/terrain/tundra_nesw.png";

    tiles[0xd0] = "assets/assets/terrain/ocean.png";

    tiles[MARBLE] = "assets/assets/terrain/marble.png"; 
    tiles[CARBON] = "assets/assets/terrain/coal.png"; 
    tiles[IRON] = "assets/assets/terrain/iron.png";
    tiles[COPPER] = "assets/assets/terrain/copper.png";
    tiles[GOLD] = "assets/assets/terrain/gold.png";
    tiles[DOE] = "assets/assets/terrain/doe.png";
    tiles[FISH] = "assets/assets/terrain/fish.png";
    tiles[GAME] = "assets/assets/terrain/game.png";
    tiles[GEMS] = "assets/assets/terrain/gems.png";
    tiles[HORSE] = "assets/assets/terrain/horse.png";
    tiles[OASIS] = "assets/assets/terrain/oasis.png";
    tiles[OIL] = "assets/assets/terrain/oil.png";
    tiles[SEAL] = "assets/assets/terrain/seal.png";
    tiles[GEOSHIELD] = "assets/assets/terrain/shield.png";
    tiles[SILVER] = "assets/assets/terrain/silver.png";
    tiles[WHALES] = "assets/assets/terrain/whales.png";
    tiles[ELEPHANTS] = "assets/assets/terrain/elephants.png";
    tiles[SILK] = "assets/assets/terrain/silk.png";
    tiles[GRAPES] = "assets/assets/terrain/grapes.png";
    tiles[SPICES] = "assets/assets/terrain/spices.png";
    tiles[SUGAR] = "assets/assets/terrain/sugar.png";
    tiles[TOBACCO] = "assets/assets/terrain/tobacco.png";
    tiles[COTTON] = "assets/assets/terrain/cotton.png";
    tiles[URANIUM] = "assets/assets/terrain/uranium.png";
    tiles[LITIUM] = "assets/assets/terrain/litium.png";
    tiles[ALUMINIUM] = "assets/assets/terrain/aluminium.png";
    tiles[HELIUM_3] = "assets/assets/terrain/helium3.png";

    // COMMODITIES icons (city UI): 7x7 pngs generated by tools/generate_commodity_icons.py
    // (task #10) from the special-resource images above, one per COMMODITIES value.
    tiles[copper]    = "assets/assets/city/copper.png";
    tiles[iron]      = "assets/assets/city/iron.png";
    tiles[silver]    = "assets/assets/city/silver.png";
    tiles[marble]    = "assets/assets/city/marble.png";
    tiles[furs]      = "assets/assets/city/furs.png";
    tiles[traan]     = "assets/assets/city/traan.png";
    tiles[gems]      = "assets/assets/city/gems.png";
    tiles[horses]    = "assets/assets/city/horses.png";
    tiles[elephants] = "assets/assets/city/elephants.png";
    tiles[silk]      = "assets/assets/city/silk.png";
    tiles[wine]      = "assets/assets/city/wine.png";
    tiles[spices]    = "assets/assets/city/spices.png";
    tiles[sugar]     = "assets/assets/city/sugar.png";
    tiles[tobacco]   = "assets/assets/city/tobacco.png";
    tiles[cotton]    = "assets/assets/city/cotton.png";
    tiles[carbon]    = "assets/assets/city/carbon.png";
    tiles[uranium]   = "assets/assets/city/uranium.png";
    tiles[oil]       = "assets/assets/city/oil.png";
    tiles[litium]    = "assets/assets/city/litium.png";
    tiles[aluminium] = "assets/assets/city/aluminium.png";
    tiles[helium_3]  = "assets/assets/city/helium_3.png";
}

// Defined here (and not in gamekernel.cpp) so that the testcase build, which does not
// link gamekernel.o, can also use the travel costs.
MovementCost movementcosts;

void initMovementCosts(MovementCost &movementcosts)
{
    movementcosts[ARCTIC] = 1.0;
    movementcosts[DESERT] = 1.0;
    movementcosts[FOREST] = 1.5;
    movementcosts[GRASSLAND] = 1.0;
    movementcosts[HILLS] = 2.0;
    movementcosts[JUNGLE] = 2.0;
    movementcosts[MOUNTAINS] = 3.0;
    movementcosts[PLAINS] = 1.0;
    movementcosts[RIVER] = 1.0;
    movementcosts[SWAMP] = 2.0;
    movementcosts[TUNDRA] = 1.5;
    movementcosts[OCEANBIOMA] = 1.0;
}

// Defined here (and not in gamekernel.cpp), same reason as movementcosts above.
ImprovementEffort improvementeffort;

// All biomas/improvements start at 6 required worker-turns; tune per bioma later.
void initImprovementEffort(ImprovementEffort &improvementeffort)
{
    int biomas[] = { ARCTIC, DESERT, FOREST, GRASSLAND, HILLS, JUNGLE, MOUNTAINS, PLAINS, RIVER, SWAMP, TUNDRA };
    int types[] = { ROAD, MINE, IRRIGATION, RAILROAD };

    for (int t : types)
        for (int b : biomas)
            improvementeffort[t][b] = 9;

    // Resource-gated improvements (see initImprovementResources): only ever buildable on
    // the biomas that can host their required special resource, so a flat effort per type
    // is enough for now; tune per bioma too, later, same as the four above if needed.
    for (int b : biomas)
    {
        improvementeffort[QUARRY][b] = 9;
        improvementeffort[CAMP][b] = 6;
        improvementeffort[DERRICK][b] = 12;
        improvementeffort[PLANTATION][b] = 6;
    }
}

int getImprovementEffort(ImprovementEffort &improvementeffort, int improvementtype, int bioma)
{
    int basebioma = bioma & 0xf0;

    auto it = improvementeffort.find(improvementtype);
    if (it != improvementeffort.end())
    {
        auto it2 = it->second.find(basebioma);
        if (it2 != it->second.end())
            return it2->second;
    }

    return 1;
}

// Defined here (and not in gamekernel.cpp), same reason as movementcosts/improvementeffort above.
ImprovementResources improvementresources;

// README.md's resource table: which special resource(s) a tile needs for each
// resource-gated improvement. Road/Irrigation/Railroad have no entry here and stay
// unrestricted -- for BUILDING purposes so does Mine (nothing calls tileHasRequiredResource
// with MINE), but its entry here still drives commodity PRODUCTION gating below
// (getRequiredImprovement): Gold/Copper/Iron/Silver/Gems/Carbon/Uranium/Litium/Aluminium/
// Helium-3 all need a mine built before their commodity is produced, per README.md's table.
void initImprovementResources(ImprovementResources &improvementresources)
{
    improvementresources[MINE]       = {GOLD, COPPER, IRON, SILVER, GEMS, CARBON, URANIUM, LITIUM, ALUMINIUM, HELIUM_3};
    improvementresources[QUARRY]     = {MARBLE};
    improvementresources[CAMP]       = {DOE, GAME, SEAL};
    improvementresources[DERRICK]    = {OIL};
    improvementresources[PLANTATION] = {GRAPES, SUGAR, TOBACCO, COTTON};
}

bool tileHasRequiredResource(ImprovementResources &improvementresources, int improvementtype, int resource)
{
    auto it = improvementresources.find(improvementtype);
    if (it == improvementresources.end())
        return true;

    for (int r : it->second)
        if (r == resource)
            return true;

    return false;
}

int getRequiredImprovement(ImprovementResources &improvementresources, int resource)
{
    for (auto &kv : improvementresources)
        for (int r : kv.second)
            if (r == resource)
                return kv.first;

    return 0;
}

void initResources(std::unordered_map<int, std::vector<int>> &resourcexbioma)
{
    resourcexbioma[ARCTIC]     = {MARBLE,GEMS,OIL,SEAL};
    resourcexbioma[DESERT]     = {OASIS,LITIUM,OIL};
    resourcexbioma[FOREST]     = {GAME,DOE};
    resourcexbioma[GRASSLAND]  = {MARBLE,CARBON,IRON,COPPER,GOLD,DOE,GAME,HORSE,COTTON,TOBACCO,GEOSHIELD};
    resourcexbioma[HILLS]      = {MARBLE,CARBON,IRON,COPPER,GOLD,GEMS,OIL,GRAPES, GEOSHIELD};
    resourcexbioma[JUNGLE]     = {IRON,GOLD,GEMS,SILK,SPICES, SUGAR, OIL};
    resourcexbioma[MOUNTAINS]  = {MARBLE,CARBON,IRON,COPPER,GOLD,SILVER,GEMS,OIL,URANIUM,ALUMINIUM,GEOSHIELD};
    resourcexbioma[PLAINS]     = {DOE,GAME,HORSE,OIL,GEOSHIELD, ELEPHANTS};
    resourcexbioma[RIVER]      = {FISH};
    resourcexbioma[SWAMP]      = {GOLD,GEMS,HELIUM_3,OIL};
    resourcexbioma[TUNDRA]     = {MARBLE,COPPER,GOLD,GEMS,OIL,SEAL};
    resourcexbioma[OCEANBIOMA] = {FISH,WHALES,OIL};
}

// Defined here (and not in gamekernel.cpp), same reason as the tables above.
std::unordered_map<int, int> commodityxresource;

const std::vector<int> ALL_COMMODITIES = {
    copper, iron, silver, marble, furs, traan, gems, horses, elephants, silk,
    wine, spices, sugar, tobacco, cotton, carbon, uranium, oil, litium, aluminium, helium_3
};

void initCommodities(std::unordered_map<int, int> &commodityxresource)
{
    commodityxresource[COPPER]    = copper;
    commodityxresource[IRON]      = iron;
    commodityxresource[SILVER]    = silver;
    commodityxresource[MARBLE]    = marble;
    commodityxresource[DOE]       = furs;
    commodityxresource[GAME]      = furs;
    commodityxresource[SEAL]      = furs;
    commodityxresource[WHALES]    = traan;
    commodityxresource[GEMS]      = gems;
    commodityxresource[HORSE]     = horses;
    commodityxresource[ELEPHANTS] = elephants;
    commodityxresource[SILK]      = silk;
    commodityxresource[GRAPES]    = wine;
    commodityxresource[SPICES]    = spices;
    commodityxresource[SUGAR]     = sugar;
    commodityxresource[TOBACCO]   = tobacco;
    commodityxresource[COTTON]    = cotton;
    commodityxresource[CARBON]    = carbon;
    commodityxresource[URANIUM]   = uranium;
    commodityxresource[OIL]       = oil;
    commodityxresource[LITIUM]    = litium;
    commodityxresource[ALUMINIUM] = aluminium;
    commodityxresource[HELIUM_3]  = helium_3;
}

#define CITY_NAMES_PER_CIVILIZATION 100

void initNaming(std::unordered_map<int,std::queue<std::string>> &citynames)
{
    std::vector<std::string> vikings = {
        "Kattegate","Jorvik","Hedeby","Trondheim","Bergen","Stavanger",
        "Kristiansand","Oslo","Stockholm","Copenhagen","Helsinki","Reykjavik",
        "Uppsala","Birka","Ribe","Aarhus","Roskilde","Odense",
        "Viborg","Aalborg","Lund","Sigtuna","Visby","Kaupang",
        "Tonsberg","Jelling","Fyrkat","Trelleborg","Torshavn","Kirkwall",
        "Dublin","Waterford","Wexford","Limerick","Cork","Rouen",
        "Holmgard","Aldeigjuborg","Skara","Hamar"
    };

    std::vector<std::string> romans = {
        "Roma","Caesarea","Carthage","Nicopolis","Byzantium","Brundisium",
        "Camulodunum","Syracuse","Antioch","Palmyra","Cyrene","Alexandria",
        "Gordion","Jerusalem","Ravenna","Artaxata",
        "Neapolis","Pompeii","Mediolanum","Ostia","Capua","Tarentum",
        "Verona","Aquileia","Londinium","Eboracum","Lutetia","Lugdunum",
        "Massilia","Tarraco","Hispalis","Corduba","Emerita","Vindobona",
        "Nicomedia","Sirmium","Salona","Utica","Leptis Magna","Hippo Regius"
    };

    std::vector<std::string> greeks = {
        "Atenas","Sparta","Corinto","Delfos","Olimpia","Micenas",
        "Tebas","Argos","Mileto","Efeso","Samos","Rodas",
        "Cnosos","Halicarnaso","Pergamo","Megara","Calcis","Eretria",
        "Mitilene","Esmirna","Focea","Naxos","Paros","Delos",
        "Egina","Patras","Larisa","Abdera","Olinto","Anfipolis",
        "Corcira","Maraton"
    };

    std::vector<std::string> chinnese = {
        "Beijing","Xian","Luoyang","Nanjing","Kaifeng","Hangzhou",
        "Chengdu","Guangzhou","Suzhou","Wuhan","Chongqing","Shenyang",
        "Datong","Dunhuang","Anyang","Handan","Linzi","Jinan",
        "Taiyuan","Yangzhou","Ningbo","Fuzhou","Quanzhou","Kunming",
        "Zhengzhou","Changsha","Nanchang","Guilin","Hefei","Jingzhou"
    };

    citynames[0] = std::queue<std::string>();       // Vikings
    citynames[1] = std::queue<std::string>();       // Romans
    citynames[2] = std::queue<std::string>();       // Greeks
    citynames[3] = std::queue<std::string>();       // Chinnese

    for(int i=0;i<CITY_NAMES_PER_CIVILIZATION;i++)
    {
        citynames[0].push(vikings[i % vikings.size()]);
        citynames[1].push(romans[i % romans.size()]);
        citynames[2].push(greeks[i % greeks.size()]);
        citynames[3].push(chinnese[i % chinnese.size()]);
    }
}

