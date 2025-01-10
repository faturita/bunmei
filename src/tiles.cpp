#include <iostream>
#include <unordered_map>
#include "tiles.h"

void initTiles(std::unordered_map<int, std::string> &tiles)
{
    tiles[OCEAN] = "assets/assets/terrain/ocean.png";
    tiles[LAND] = "assets/assets/terrain/land.png";

    tiles[RIVER_MOUTH_W] = "assets/assets/terrain/river_mouth_w.png";
    tiles[RIVER_MOUTH_S] = "assets/assets/terrain/river_mouth_s.png";
    tiles[RIVER_MOUTH_E] = "assets/assets/terrain/river_mouth_e.png";
    tiles[RIVER_MOUTH_N] = "assets/assets/terrain/river_mouth_n.png";

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
    tiles[COAL] = "assets/assets/terrain/coal.png"; 
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
}

void initCommodities(std::unordered_map<int, std::vector<int>> &commodities)
{
    commodities[ARCTIC]     = {MARBLE,GEMS,OIL,SEAL};
    commodities[DESERT]     = {OASIS,OIL};
    commodities[FOREST]     = {GAME};
    commodities[GRASSLAND]  = {MARBLE,COAL,IRON,COPPER,GOLD,DOE,GAME,HORSE,OIL,GEOSHIELD};
    commodities[HILLS]      = {MARBLE,COAL,IRON,COPPER,GOLD,GEMS,OIL,GEOSHIELD};
    commodities[JUNGLE]     = {IRON,GOLD,GEMS,OIL};
    commodities[MOUNTAINS]  = {MARBLE,COAL,IRON,COPPER,GOLD,GEMS,OIL,GEOSHIELD};
    commodities[PLAINS]     = {DOE,GAME,HORSE,OIL,GEOSHIELD};
    commodities[RIVER]      = {FISH};
    commodities[SWAMP]      = {GOLD,GEMS,OIL};
    commodities[TUNDRA]     = {MARBLE,COPPER,GOLD,GEMS,OIL,SEAL};
    commodities[OCEANBIOMA] = {FISH,OIL};
}

void initNaming(std::unordered_map<int,std::queue<std::string>> &citynames)
{
    citynames[0] = std::queue<std::string>();       // Vikings
    citynames[1] = std::queue<std::string>();       // Romans
    citynames[2] = std::queue<std::string>();       // Greeks

    citynames[0].push("Kattegate");
    citynames[0].push("Jorvik");
    citynames[0].push("Hedeby");
    citynames[0].push("Trondheim");
    citynames[0].push("Bergen");
    citynames[0].push("Stavanger");
    citynames[0].push("Kristiansand");
    citynames[0].push("Oslo");
    citynames[0].push("Stockholm");
    citynames[0].push("Copenhagen");
    citynames[0].push("Helsinki");
    citynames[0].push("Reykjavik");

    citynames[1].push("Roma");
    citynames[1].push("Caesarea");
    citynames[1].push("Carthage");
    citynames[1].push("Nicopolis");
    citynames[1].push("Byzantium");
    citynames[1].push("Brundisium");
    citynames[1].push("Camulodunum");
    citynames[1].push("Syracuse");
    citynames[1].push("Antioch");
    citynames[1].push("Palmyra");
    citynames[1].push("Cyrene");
    citynames[1].push("Alexandria");
    citynames[1].push("Gordion");
    citynames[1].push("Jerusalem");
    citynames[1].push("Ravenna");
    citynames[1].push("Artaxata");

    citynames[2].push("Atenas");
    citynames[2].push("Sparta");
    citynames[2].push("Corinto");
    citynames[2].push("Tevas");
    citynames[2].push("Delfos");
    citynames[2].push("Olimpia");
    citynames[2].push("Micenas");
    citynames[2].push("Tebas");
    citynames[2].push("Argos");
    citynames[2].push("Mileto");
    citynames[2].push("Efeso");
    citynames[2].push("Samos");
    citynames[2].push("Rodas");    
}

