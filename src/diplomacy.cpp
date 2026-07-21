#include "diplomacy.h"

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

void initDiplomacy(std::vector<std::vector<Diplomacy>> &diplomacy, int numberoffactions)
{
    diplomacy.assign(numberoffactions, std::vector<Diplomacy>(numberoffactions));

    for (int i = 0; i < numberoffactions; i++)
        for (int j = 0; j < numberoffactions; j++)
        {
            Diplomacy d;
            d.status = NO_CONTACT;
            statusFlags(d.status, d.landSeizure, d.openBorders);
            diplomacy[i][j] = d;
        }
}
