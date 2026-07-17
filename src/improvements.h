#ifndef IMPROVEMENTS_H
#define IMPROVEMENTS_H

#include <string.h>
#include <unordered_map>

enum IMPROVEMENT_TYPES {
    IRRIGATION = 0x01,
    MINE = 0x02,
    ROAD = 0x04,
    RAILROAD = 0x08
};


class Improvement {
    public:
    int id;
    int type;
    char assetname[256];
    char name[256];

    Improvement(int idnew, int typenew, const char* assetnamenew, const char* namenew)
    {
        id = idnew;
        type = typenew;
        strcpy(assetname,assetnamenew);
        strcpy(name,namenew);
    }

};

void initImprovements(std::unordered_map<int, Improvement*>& improvements);

#endif // IMPROVEMENTS_H