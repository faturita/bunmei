#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include "../shippable.h"

class Transport // Interface
{
    public:
        virtual bool board(Shippable* passenger) = 0;
        virtual Shippable* unboard() = 0;
        virtual int manifest() = 0;

};

#endif   // TRANSPORT_H