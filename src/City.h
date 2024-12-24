#ifndef CITY_H
#define CITY_H

#include <iostream>

class City
{
    public:
        City();
        int latitude;
        int longitude;
        int faction;
        int id;
        int pop;
        char name[256];

    void virtual draw();

};

#endif // CITY_H