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
    aw = 2;
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



