#ifndef FACTION_H
#define FACTION_H

#include <vector>

class Faction {
    protected:
        bool doneThisTurn;
    public:
        int pop;
        int coins;
        char name[256];
        int id;
        int red, green, blue;

        bool autoPlayer = true; //@NOTE Change this if you want to be able to control the other factions.

        int mapoffset;
        int vmapoffset;

        int blinkingrate = 70;

        int p=0;

        float rates[4];

    Faction ()
    {
        doneThisTurn = false;
        mapoffset = vmapoffset=0;
    };

    void done()
    {
        doneThisTurn = true;
    };

    bool isDone()
    {
        return doneThisTurn;
    };

    void ready()
    {
        doneThisTurn = false;
        p=0;
    };
};

typedef std::vector<Faction*> Factions;

#endif // FACTION_H
