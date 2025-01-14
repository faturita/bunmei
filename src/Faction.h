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

        bool autoPlayer = true;

        int mapoffset;

        int blinkingrate = 70;

        int p=0;

        float rates[4];

    Faction ()
    {
        doneThisTurn = false;
        mapoffset = 0;
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
