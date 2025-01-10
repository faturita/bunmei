#ifndef RESOURCES_H
#define RESOURCES_H

#include <string.h>

enum RESOURCE_TYPES {
    FOOD,
    SHIELDS,
    TRADE,
    COINS,
    SCIENCE,
    CULTURE
};


class Resource {
    public:
    int id;
    int type;
    char assetname[256];
    char name[256];

    Resource(int idnew, int typenew, const char* assetnamenew, const char* namenew)
    {
        id = idnew;
        type = typenew;
        strcpy(assetname,assetnamenew);
        strcpy(name,namenew);
    }

};

void initResources(std::vector<Resource*> &resources);

#endif // RESOURCES_H