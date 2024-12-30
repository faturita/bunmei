#ifndef BUILDABLE_H
#define BUILDABLE_H

enum BuildableType {
    UNIT = 0,
    BUILDING = 1
};

class Buildable {
    public:
    virtual BuildableType getType() = 0;
};

class BuildableFactory {
public:
    virtual int cost(int r_id) = 0;
    virtual Buildable* create() = 0;
};





#endif // BUILDABLE_H