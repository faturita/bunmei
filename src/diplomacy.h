#ifndef DIPLOMACY_H
#define DIPLOMACY_H

#include <vector>

// The relation between two factions, from README.md "Diplomacy and Wars": a transition
// graph/finite state machine, one state per (faction,faction) pair.  Each state fixes
// whether a unit moving into land owned by the OTHER faction seizes it (landSeizure) and/or
// is even allowed to walk in without seizing it (openBorders).
enum DiplomaticStatus
{
    NO_CONTACT              = 0,
    FOE                      = 1,
    CEASEFIRE                = 2,
    ARMISTICE                = 3,
    PEACE                    = 4,
    NON_AGGRESSION_AGREEMENT = 5,
    TRADE_AGREEMENT          = 6,
    COALITION                = 7,
    VASSALAGE                = 8
};

struct Diplomacy
{
    int status;
    bool landSeizure;
    bool openBorders;
};

// Fills diplomacy[numberoffactions][numberoffactions], one entry per (faction,faction) pair,
// all starting at NO_CONTACT (landSeizure=true, openBorders=true per the DefCon table).
void initDiplomacy(std::vector<std::vector<Diplomacy>> &diplomacy, int numberoffactions);

#endif // DIPLOMACY_H
