#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Trireme.h"

extern std::vector<Faction*> factions;

Trireme::Trireme()
{
    strcpy(name,"Trireme");
    strcpy(assetname,"assets/assets/units/trireme.png");
    moves = 4;
    aw = 2;
}


MOVEMENT_TYPE Trireme::getMovementType()
{
    return OCEANTYPE;
}

bool Trireme::board(Shippable* passenger)
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

Shippable* Trireme::unboard()
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

int Trireme::manifest()
{
    return passengers.size();
}

int Trireme::capacity()
{
    return cargo;
}

Shippable* Trireme::findCargo(int id)
{
    for (Shippable* passenger : passengers)
        if (passenger->getId() == id)
            return passenger;
    return nullptr;
}

std::vector<Shippable*> Trireme::getCargo()
{
    return passengers;
}

Unit* Trireme::unboardUnit()
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

bool Trireme::removeCargo(int id)
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

void Trireme::update(int newlat, int newlon)
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

Trireme* TriremeFactory::create()
{
    return new Trireme();
}

int Trireme::getSubType()
{
    return UNIT_TRIREME;
}

TriremeFactory::TriremeFactory()
{
    strncpy(this->name,"Trireme",256);
    addDependencyCode(TECH_MAP_MAKING);
}

int TriremeFactory::cost(int r_id)
{
    return 40;
}



