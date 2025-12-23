#ifndef COORDINATOR_H
#define COORDINATOR_H

#include "commandorder.h"

struct Coordinator 
{
    private:
    CommandOrder corder;

    public:
    // Unit id of the active unit
    int a_u_id;
    // Faction id of the active player
    int a_f_id;
    bool endofturn = false;

    void push(CommandOrder co)
    {
        corder = co;
    }

    CommandOrder pop()
    {
        CommandOrder cr = corder;

        corder.command = Command::None;

        return cr;
    }


    void reset()
    {
        corder.command = Command::None;
    }
};

#endif // COORDINATOR_H
