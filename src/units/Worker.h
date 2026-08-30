#ifndef WORKER_H
#define WORKER_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Worker : public Unit, public Shippable
{
    public:
    Worker();
    int getSubType();
    bool canBuildCity();
    int getId() override;
    const char* getName() override;
};

class WorkerFactory : public BuildableFactory
{
    public:
    WorkerFactory();
    Worker* create();
    virtual int cost(int r_id);
};

#endif   // WORKER_H