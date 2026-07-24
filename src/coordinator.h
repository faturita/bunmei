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

    // Faction id of the active controlling player
    int a_f_id;

    // Faction id of the visible controlling player (the view that is presented on the screen)
    int v_f_id;

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
