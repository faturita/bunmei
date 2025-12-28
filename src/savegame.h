#ifndef SAVEGAME_H
#define SAVEGAME_H



void savegame(const char* filename);
void loadCities(std::ifstream& in);
void loadUnits(std::ifstream& in);


#endif // SAVEGAME_H