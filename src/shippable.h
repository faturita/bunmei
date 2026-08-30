#ifndef SHIPPABLE_H
#define SHIPPABLE_H

// @NOTE: A pure virtual here is NOT satisfied by an unrelated sibling base's matching method
// (e.g. Unit::getId() in "class X : public Unit, public Shippable") -- each such X must define its own.
class Shippable
{
    public:
        virtual ~Shippable() {}
        virtual int getId() = 0;
        virtual const char* getName() = 0;

};



#endif // SHIPPABLE_H