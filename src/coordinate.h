#ifndef COORDINATE_H
#define COORDINATE_H

struct coordinate
{
    coordinate(int lat,int lon)
    {
        this->lat = lat;
        this->lon = lon;
    }
    int lat;
    int lon;
};

#endif // COORDINATE_H
