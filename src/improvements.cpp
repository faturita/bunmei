#include "improvements.h"

void initImprovements(std::unordered_map<int, Improvement*>& improvements)
{
    improvements[0xe0] = new Improvement(0xe0, IRRIGATION, "assets/assets/improvements/irrigation.png", "Irrigation");
    improvements[0xf0] = new Improvement(0xf0, MINE, "assets/assets/improvements/mine.png", "Mine");

    improvements[0xe1] = new Improvement(0xe1, ROAD, "assets/assets/improvements/road_w.png", "Road");
    improvements[0xe2] = new Improvement(0xe2, ROAD, "assets/assets/improvements/road_s.png", "Road");
    improvements[0xe3] = new Improvement(0xe3, ROAD, "assets/assets/improvements/road_sw.png", "Road");
    improvements[0xe4] = new Improvement(0xe4, ROAD, "assets/assets/improvements/road_e.png", "Road");
    improvements[0xe6] = new Improvement(0xe6, ROAD, "assets/assets/improvements/road_se.png", "Road");
    improvements[0xe8] = new Improvement(0xe8, ROAD, "assets/assets/improvements/road_n.png", "Road");
    improvements[0xe9] = new Improvement(0xe9, ROAD, "assets/assets/improvements/road_nw.png", "Road");
    improvements[0xec] = new Improvement(0xec, ROAD, "assets/assets/improvements/road_ne.png", "Road");

    improvements[0xf1] = new Improvement(0xf1, RAILROAD, "assets/assets/improvements/railroad_w.png", "Railroad");
    improvements[0xf2] = new Improvement(0xf2, RAILROAD, "assets/assets/improvements/railroad_s.png", "Railroad");
    improvements[0xf3] = new Improvement(0xf3, RAILROAD, "assets/assets/improvements/railroad_sw.png", "Railroad");
    improvements[0xf4] = new Improvement(0xf4, RAILROAD, "assets/assets/improvements/railroad_e.png", "Railroad");
    improvements[0xf6] = new Improvement(0xf6, RAILROAD, "assets/assets/improvements/railroad_se.png", "Railroad");
    improvements[0xf8] = new Improvement(0xf8, RAILROAD, "assets/assets/improvements/railroad_n.png", "Railroad");
    improvements[0xf9] = new Improvement(0xf9, RAILROAD, "assets/assets/improvements/railroad_nw.png", "Railroad");
    improvements[0xfc] = new Improvement(0xfc, RAILROAD, "assets/assets/improvements/railroad_ne.png", "Railroad");

}