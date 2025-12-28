#include "../openglutils.h"
#include "../map.h"
#include "../Faction.h"
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

bool Trireme::board(Unit* passenger)
{
    if (cargo-passengers.size()>0)
    {
        passengers[passenger->id] = passenger;
        return true;
    }
    else
    {
        return false;
    }
}

Unit* Trireme::unboard()
{
    if (passengers.size()>0)
    {
        auto it = passengers.begin();
        Unit* passenger = it->second;
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
        printf("Moving what I am transporting %s\n",passenger->name);
        passenger->update(newlat,newlon);
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
}

int TriremeFactory::cost(int r_id)
{
    return 40;
}



