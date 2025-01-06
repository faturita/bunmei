#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "Worker.h"

extern std::vector<Faction*> factions;

Worker::Worker()
{
    strcpy(name,"Worker");
    moves = 2;
    aw = 0;
    dw = 0;
}

void Worker::draw()
{
    int red = factions[faction]->red;
    int green = factions[faction]->green;
    int blue = factions[faction]->blue;
    
    placeThisUnit(latitude,longitude,16,"assets/assets/units/worker.png", red, green, blue);
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