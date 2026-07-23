#include "diplomacy.h"

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
