#ifndef CITY_H
#define CITY_H

#include <iostream>

class City
{
    public:
        int latitude;
        int longitude;
        int faction;
        int id;
        int pop;

    void virtual draw();

};

#endif // CITY_H