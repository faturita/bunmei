#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
#include "../codes.h"
#include "Galleon.h"

extern std::vector<Faction*> factions;

Galleon::Galleon()
{
    strcpy(name,"Galleon");
    strcpy(assetname,"assets/assets/units/galleon.png");
    moves = 4;
    aw = 4;
}


MOVEMENT_TYPE Galleon::getMovementType()
{
    return OCEANTYPE;
}

bool Galleon::board(Shippable* passenger)
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

Shippable* Galleon::unboard()
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

int Galleon::manifest()
{
    return passengers.size();
}

int Galleon::capacity()
{
    return cargo;
}

Shippable* Galleon::findCargo(int id)
{
    for (Shippable* passenger : passengers)
        if (passenger->getId() == id)
            return passenger;
    return nullptr;
}

std::vector<Shippable*> Galleon::getCargo()
{
    return passengers;
}

Unit* Galleon::unboardUnit()
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

bool Galleon::removeCargo(int id)
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

void Galleon::update(int newlat, int newlon)
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

Galleon* GalleonFactory::create()
{
    return new Galleon();
}

int Galleon::getSubType()
{
    return UNIT_GALLEON;
}

GalleonFactory::GalleonFactory()
{
    strncpy(this->name,"Galleon",256);
    addDependencyCode(TECH_MAP_MAKING);
}

int GalleonFactory::cost(int r_id)
{
    return 200;
}



