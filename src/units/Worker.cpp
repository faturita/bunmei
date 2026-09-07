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

int Worker::getId()
{
    return id;
}

const char* Worker::getName()
{
    return name;
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

std::vector<int> WorkerFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> WorkerFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 40)
    {
        consumedResources.push_back(new Resource{SHIELDS, 40});
    }
    return consumedResources;
}

