#ifndef CITY_H
#define CITY_H

#include <unordered_map>
#include <iostream>
#include "coordinate.h"

class City
{
    protected:
    std::unordered_map<coordinate, int> tiles;
    public:
    std::vector<int> resources;
    
        City();
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

    void tick();

    bool workingOn(int lat, int lon);

};

#endif // CITY_H