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
    if (cargo-passengers.size()>0)
    {
        passengers[passenger->getId()] = passenger;
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
        auto it = passengers.begin();
        Shippable* passenger = it->second;
        passengers.erase(it);
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
    auto it = passengers.find(id);
    return it!=passengers.end() ? it->second : nullptr;
}

std::vector<Shippable*> Galleon::getCargo()
{
    std::vector<Shippable*> list;
    for (auto& [k, passenger] : passengers)
        list.push_back(passenger);
    return list;
}

Unit* Galleon::unboardUnit()
{
    for (auto it = passengers.begin(); it != passengers.end(); it++)
    {
        if (Unit* u = dynamic_cast<Unit*>(it->second))
        {
            passengers.erase(it);
            return u;
        }
    }
    return nullptr;
}

bool Galleon::removeCargo(int id)
{
    return passengers.erase(id) > 0;
}

void Galleon::update(int newlat, int newlon)
{
    oldlatitude = latitude;
    oldlongitude = longitude;

    latitude = newlat;
    longitude = newlon;

    completion = 0;
    fortified = false;

    for(auto& [k, passenger]:passengers)
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



