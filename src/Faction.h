#ifndef FACTION_H
#define FACTION_H

class Faction {
    protected:
        bool doneThisTurn;
    public:
        int pop;
        char name[256];
        int id;
        int red, green, blue;

    Faction ()
    {
        doneThisTurn = false;
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
