#ifndef RESOURCES_H
#define RESOURCES_H

#include <string.h>
#include <vector>
#include "shippable.h"

enum CORE_RESOURCES {
    FOOD        = 0x00,
    SHIELDS     = 1,
    TRADE       = 2,
    COINS       = 3,
    SCIENCE     = 4,
    CULTURE     = 5
};

enum COMMODITIES
{
    copper     = 0x201,
    iron       = 0x202,
    silver     = 0x203,
    marble     = 0x204,
    furs       = 0x205,
    traan      = 0x206,
    gems       = 0x207,
    meat       = 0x208,
    horses     = 0x209,
    elephants  = 0x20a,
    silk       = 0x20b,
    wine       = 0x20c,
    spices     = 0x20d,
    gunpowder  = 0x20e,
    sugar      = 0x20f,
    tobacco    = 0x210,
    cotton     = 0x211,
    carbon     = 0x212,
    uranium    = 0x213,
    oil        = 0x214,
    litium     = 0x215,
    aluminium  = 0x216,
    helium_3   = 0x217
};

enum MFGOODS
{
    tools           = 0x301,
    guns            = 0x302,
    textiles        = 0x303,
    steel           = 0x304,
    automobiles     = 0x305,
    plastics        = 0x306,
    pharmaceuticals = 0x307,
    electronics     = 0x308,
    robotics        = 0x309
};

class Resource
{ 
public:
    virtual ~Resource() {}
    int amount;
};


class CoreResource : public Resource {
    public:
    int id;
    char assetname[256];
    char name[256];

    CoreResource(int idnew, const char* assetnamenew, const char* namenew)
    {
        id = idnew;
        strcpy(assetname,assetnamenew);
        strcpy(name,namenew);
    }
};


class Commodity : public Resource, public Shippable {
    public:
    int id;
    char assetname[256];
    char name[256];

    Commodity(int idnew, const char* assetnamenew, const char* namenew)
    {
        id = idnew;
        strcpy(assetname,assetnamenew);
        strcpy(name,namenew);
    }

    int getId() override { return id; }
    const char* getName() override { return name; }
};

class MfgGood : public Resource, public Shippable {
    public:
    int id;
    char assetname[256];
    char name[256];

    MfgGood(int idnew, const char* assetnamenew, const char* namenew)
    {
        id = idnew;
        strcpy(assetname,assetnamenew);
        strcpy(name,namenew);
    }

    int getId() override { return id; }
    const char* getName() override { return name; }
};

// @FIXME: Hardcode the list of resources, commodiies and manufactured goods.  This should be loaded from a file.
const int ALL_CORE_RESOURCES[] = {FOOD,SHIELDS,TRADE,COINS,SCIENCE,CULTURE};

const int ALL_COMMODITIES[] = {copper, iron, silver, marble, furs, traan, gems, meat, horses, elephants, silk, wine, spices, gunpowder, sugar, tobacco, cotton, carbon, uranium, oil, litium, aluminium, helium_3};

const int ALL_MFG_GOODS[] = {tools, guns, textiles, steel, automobiles, plastics, pharmaceuticals, electronics, robotics};

#endif // RESOURCES_H