#include "diplomacy.h"

void initDiplomacy(DiplomacyTable &diplomacy, int numberoffactions)
{
    diplomacy.resize(numberoffactions);

    for (int i = 0; i < numberoffactions; i++)
        for (int j = i; j < numberoffactions; j++)
        {
            Diplomacy d;
            d.status = NO_CONTACT;
            statusFlags(d.status, d.landSeizure, d.openBorders);
            diplomacy[i][j] = d;
        }
}

void growDiplomacy(DiplomacyTable &diplomacy, int numberoffactions)
{
    int old = diplomacy.size();
    diplomacy.grow(numberoffactions);

    for (int i = 0; i < numberoffactions; i++)
        for (int j = std::max(i, old); j < numberoffactions; j++)
        {
            Diplomacy d;
            d.status = NO_CONTACT;
            statusFlags(d.status, d.landSeizure, d.openBorders);
            diplomacy[i][j] = d;
        }
}
