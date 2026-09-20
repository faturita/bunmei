#ifndef SAVEGAME_H
#define SAVEGAME_H



void savegame(const char* filename);
void loadCities(std::ifstream& in);
void loadUnits(std::ifstream& in);

// Restore the Dependency Evaluation Engine registry (world/faction/city scopes alike) and each
// faction's tech-graph progress. Called from gamekernel.cpp:loadWorldModelling() after the
// cities and units, in the order savegame() wrote them.
void loadDependencies(std::ifstream& in);
void loadTechnologies(std::ifstream& in);


#endif // SAVEGAME_H