#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include "Unit.h"


class Transport // Interface
{
    public:
        virtual bool board(Unit* passenger) = 0;
        virtual Unit* unboard() = 0;
        virtual int manifest() = 0;

};

#endif   // TRANSPORT_H