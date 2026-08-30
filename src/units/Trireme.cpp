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

Shippable* Trireme::unboard()
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
    auto it = passengers.find(id);
    return it!=passengers.end() ? it->second : nullptr;
}

std::vector<Shippable*> Trireme::getCargo()
{
    std::vector<Shippable*> list;
    for (auto& [k, passenger] : passengers)
        list.push_back(passenger);
    return list;
}

Unit* Trireme::unboardUnit()
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

bool Trireme::removeCargo(int id)
{
    return passengers.erase(id) > 0;
}

void Trireme::update(int newlat, int newlon)
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



