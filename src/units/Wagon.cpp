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

Shippable* Wagon::unboard()
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
    auto it = passengers.find(id);
    return it!=passengers.end() ? it->second : nullptr;
}

std::vector<Shippable*> Wagon::getCargo()
{
    std::vector<Shippable*> list;
    for (auto& [k, passenger] : passengers)
        list.push_back(passenger);
    return list;
}

Unit* Wagon::unboardUnit()
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

bool Wagon::removeCargo(int id)
{
    return passengers.erase(id) > 0;
}

void Wagon::update(int newlat, int newlon)
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



