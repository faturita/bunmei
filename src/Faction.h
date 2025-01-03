#ifndef FACTION_H
#define FACTION_H

class Faction {
    protected:
        bool doneThisTurn;
    public:
        int pop;
        int gold;
        char name[256];
        int id;
        int red, green, blue;

        bool autoPlayer = true;

        int mapoffset;

        int blinkingrate = 70;

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
    };
};

#endif // FACTION_H
