#ifndef WORKER_H
#define WORKER_H

#include <iostream>
#include "Unit.h"


class Worker : public Unit
{
    public:
    Worker();

    bool canBuildCity();
};

class WorkerFactory : public BuildableFactory
{
    public:
    WorkerFactory();
    Worker* create();
    virtual int cost(int r_id);
};

#endif   // WORKER_H