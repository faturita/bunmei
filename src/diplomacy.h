#ifndef DIPLOMACY_H
#define DIPLOMACY_H

#include <vector>
#include <algorithm>

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

// The DefCon table from README.md "Diplomacy and Wars": what each status implies for land
// seizure and open borders.
static void statusFlags(int status, bool &landSeizure, bool &openBorders)
{
    switch (status)
    {
        case NO_CONTACT:                landSeizure = true;  openBorders = true;  break;
        case FOE:                       landSeizure = true;  openBorders = true;  break;
        case CEASEFIRE:                 landSeizure = false; openBorders = false; break;
        case ARMISTICE:                 landSeizure = false; openBorders = false; break;
        case PEACE:                     landSeizure = false; openBorders = false; break;
        case NON_AGGRESSION_AGREEMENT:  landSeizure = false; openBorders = false; break;
        case TRADE_AGREEMENT:           landSeizure = false; openBorders = true;  break;
        case COALITION:                 landSeizure = false; openBorders = true;  break;
        case VASSALAGE:                 landSeizure = false; openBorders = true;  break;
        default:                        landSeizure = false; openBorders = false; break;
    }
}

struct Diplomacy
{
    int status;
    bool landSeizure;
    bool openBorders;


    void makePeace()
    {
        status = PEACE;
        statusFlags(status, landSeizure, openBorders);
    }

    void makeWar()
    {
        status = FOE;
        statusFlags(status, landSeizure, openBorders);
    }
};

// A (faction,faction) relation table where diplomacy[f1][f2] and diplomacy[f2][f1] are
// always the SAME entry: the relation is undirected (one status per unordered pair), so
// callers never need to write both sides by hand to keep them in sync.  Storage is a
// triangular array indexed by the canonical (min,max) pair; operator[] returns a proxy
// that resolves diplomacy[f1][f2] to that shared entry.
class DiplomacyTable
{
public:
    void resize(int n)
    {
        numberoffactions = n;
        entries.assign(n * (n + 1) / 2, Diplomacy());
    }

    Diplomacy& at(int f1, int f2)
    {
        int i = std::min(f1, f2);
        int j = std::max(f1, f2);
        return entries[i * numberoffactions - (i * (i - 1)) / 2 + (j - i)];
    }

    struct Row
    {
        DiplomacyTable *table;
        int f1;
        Diplomacy& operator[](int f2) { return table->at(f1, f2); }
    };

    Row operator[](int f1) { return Row{this, f1}; }

private:
    int numberoffactions = 0;
    std::vector<Diplomacy> entries;
};

// Fills diplomacy for numberoffactions factions, one entry per UNORDERED (faction,faction)
// pair, all starting at NO_CONTACT (landSeizure=true, openBorders=true per the DefCon table).
void initDiplomacy(DiplomacyTable &diplomacy, int numberoffactions);

#endif // DIPLOMACY_H
