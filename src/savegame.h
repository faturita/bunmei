#ifndef SAVEGAME_H
#define SAVEGAME_H

#include <istream>
#include <string>

void savegame(const char* filename);

// What the savegame header said about the file that produced it.
struct SaveGameInfo
{
    unsigned int savegameVersion = 0;   // the on-disk format version
    std::string  gameVersion;           // BUNMEI::version of the build that wrote it
};

// Opens `filename`, checks its magic, length and MD5, and refuses a format version this
// build does not speak; on success `data` holds the verified savegame DATA (the payload past
// its header) for the load* functions below to read through an istringstream.
//
// Returns false on ANY of those failures, having printed the reason. A caller that gets false
// must load nothing at all -- the point of the check is to avoid a half-applied world.
bool readSaveGame(const char* filename, std::string& data, SaveGameInfo& info);
void loadCities(std::istream& in);
void loadUnits(std::istream& in);

// Restore the Dependency Evaluation Engine registry (world/faction/city scopes alike) and each
// faction's tech-graph progress. Called from gamekernel.cpp:loadWorldModelling() after the
// cities and units, in the order savegame() wrote them.
void loadDependencies(std::istream& in);
void loadTechnologies(std::istream& in);

// Restore each unit's passive state (fortified/sentried/automated with its destination), the
// improvement it was building -- restarted from full effort, the counters are not saved -- and
// everything aboard a Transport: boarded units are re-boarded by id, resource cargo is rebuilt.
// Last block in the file, so a savegame written before it existed just hits EOF and no-ops.
void loadUnitStatus(std::istream& in);


#endif // SAVEGAME_H