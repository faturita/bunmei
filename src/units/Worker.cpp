#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Worker.h"

extern std::vector<Faction*> factions;

Worker::Worker()
{
    strcpy(name,"Worker");
    strcpy(assetname,"assets/assets/units/worker.png");
    moves = 2;
    aw = 0;
    dw = 0;
}

int Worker::getSubType()
{
    return UNIT_WORKER;
}

bool Worker::canBuildCity()
{
    return false;
}

Worker* WorkerFactory::create()
{
    return new Worker();
}

WorkerFactory::WorkerFactory()
{
    strncpy(this->name,"Worker",256);  
}

int WorkerFactory::cost(int r_id)
{
    return 40;
}

