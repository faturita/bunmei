#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Wagon.h"

extern std::vector<Faction*> factions;

Wagon::Wagon()
{
    strcpy(name,"Wagon");
    strcpy(assetname,"assets/assets/units/wagon.png");
    moves = 2;
    aw = 0;
    dw = 0;
}


MOVEMENT_TYPE Wagon::getMovementType()
{
    return LANDTYPE;
}

bool Wagon::board(Shippable* passenger)
{
    if ((int)passengers.size() < cargo)
    {
        passengers.push_back(passenger);
        return true;
    }
    else
    {
        return false;
    }
}

Shippable* Wagon::unboard()
{
    if (passengers.size()>0)
    {
        Shippable* passenger = passengers.front();
        passengers.erase(passengers.begin());
        return passenger;
    }
    else
    {
        return nullptr;
    }
}

int Wagon::manifest()
{
    return passengers.size();
}

int Wagon::capacity()
{
    return cargo;
}

Shippable* Wagon::findCargo(int id)
{
    for (Shippable* passenger : passengers)
        if (passenger->getId() == id)
            return passenger;
    return nullptr;
}

std::vector<Shippable*> Wagon::getCargo()
{
    return passengers;
}

Unit* Wagon::unboardUnit()
{
    for (auto it = passengers.begin(); it != passengers.end(); it++)
    {
        if (Unit* u = dynamic_cast<Unit*>(*it))
        {
            passengers.erase(it);
            return u;
        }
    }
    return nullptr;
}

bool Wagon::removeCargo(int id)
{
    for (auto it = passengers.begin(); it != passengers.end(); it++)
    {
        if ((*it)->getId() == id)
        {
            passengers.erase(it);
            return true;
        }
    }
    return false;
}

void Wagon::update(int newlat, int newlon)
{
    oldlatitude = latitude;
    oldlongitude = longitude;

    latitude = newlat;
    longitude = newlon;

    completion = 0;
    fortified = false;

    for (Shippable* passenger : passengers)
    {
        printf("Moving what I am transporting %s\n",passenger->getName());

        if (Unit* u = dynamic_cast<Unit*>(passenger))
        {
            u->update(newlat,newlon);
        }

    }

}

// ----------------------------

Wagon* WagonFactory::create()
{
    return new Wagon();
}

int Wagon::getSubType()
{
    return UNIT_WAGON;
}

WagonFactory::WagonFactory()
{
    strncpy(this->name,"Wagon",256);
    addDependencyCode(TECH_THE_WHEEL);
}

int WagonFactory::cost(int r_id)
{
    return 40;
}



