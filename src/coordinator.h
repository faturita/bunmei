#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <queue>

#include "commandorder.h"

extern int year;

struct Coordinator
{
    private:
    std::queue<CommandOrder> corder;

    public:
    // Unit id of the active unit
    int a_u_id;

    // Faction id of the active controlling player
    int a_f_id;

    // Faction id of the visible controlling player (the view that is presented on the screen)
    int v_f_id;

    bool endofturn = false;

    // The id the next issued command gets (CommandOrder::id).
    int nextorderid = 1;

    // Issues a command: stamps it with the next id and the current year. A command that
    // already has an id (read back from a match log, or sent by another player) keeps it.
    void push(CommandOrder co)
    {
        if (co.id == 0)
        {
            co.id = nextorderid++;
            co.year = year;
        }
        corder.push(co);
    }

    // Pops and returns the oldest pending command. If the queue is empty, returns a
    // Command::None order rather than being undefined behavior (std::queue::front() on
    // an empty queue).
    CommandOrder pop()
    {
        if (corder.empty())
        {
            CommandOrder none;
            none.command = Command::None;
            return none;
        }

        CommandOrder cr = corder.front();
        corder.pop();

        return cr;
    }

    bool empty()
    {
        return corder.empty();
    }


    void reset()
    {
        while (!corder.empty())
            corder.pop();
    }
};

#endif // COORDINATOR_H
