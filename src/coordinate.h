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

    // Overload the == operator
    bool operator==(const coordinate& c) const
    {
        return lat == c.lat && lon == c.lon;
    }
};

// Extend the coordinate class to include a hash function
namespace std
{
    template <>
    struct hash<coordinate>
    {
        std::size_t operator()(const coordinate& c) const
        {
            return std::hash<int>()(c.lat) ^ std::hash<int>()(c.lon);
        }
    };
}

#endif // COORDINATE_H
