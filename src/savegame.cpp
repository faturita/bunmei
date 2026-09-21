
#include <fstream>
#include <sstream>
#include <filesystem>

#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"
#include "coordinator.h"
#include "dee.h"
#include "technologies.h"

#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"
#include "units/Archer.h"
#include "units/Horsearcher.h"
#include "units/Horseman.h"
#include "units/Chariot.h"
#include "units/Trireme.h"
#include "units/Galleon.h"
#include "units/Galley.h"
#include "units/Worker.h"
#include "units/Swordman.h"
#include "units/Spearman.h"
#include "units/Axeman.h"

#include "buildings/Building.h"
#include "buildings/Palace.h"
#include "buildings/Barracks.h"
#include "buildings/Granary.h"
#include "buildings/Collosseum.h"




#include "messages.h"
#include "engine.h"

#include "tiles.h"

#include "md5.h"
#include "version.h"

#include "savegame.h"

extern std::vector<Faction*> factions;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;

extern std::unordered_map<int,std::queue<std::string>> citynames;

extern Map map;

extern int year;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;

extern Tiles tiles;                          // resource id -> icon path, for rebuilt cargo
extern ImprovementEffort improvementeffort;  // how long an improvement takes, per bioma

// ---------------------------------------------------------------------------------------
// Dependency Evaluation Engine and tech graph.
//
// Both are written as plain (count, then that many records) blocks with no fixed-size tables
// and no positional indices -- every record names what it is (a contextId, a technology code).
// That is deliberate: the tech table and the code set are expected to keep changing, and this
// way adding or removing a technology only costs the unknown records on load, not the file
// format.

// The DEE registry verbatim: contextId -> the codes registered against it. contextId already
// encodes world / faction / city scope (dee.cpp), so all three levels come along for free.
static void saveDependencies(std::ostream& out)
{
    const auto& registry = dee.getRegistry();

    size_t context_count = registry.size();
    out.write(reinterpret_cast<const char*>(&context_count), sizeof(context_count));

    for (const auto& entry : registry)
    {
        int contextId = entry.first;
        size_t code_count = entry.second.size();
        out.write(reinterpret_cast<const char*>(&contextId), sizeof(contextId));
        out.write(reinterpret_cast<const char*>(&code_count), sizeof(code_count));
        for (int codeId : entry.second)
            out.write(reinterpret_cast<const char*>(&codeId), sizeof(codeId));
    }
}

// Only each faction's PROGRESS. The graph itself -- nodes, edges, README weights, depth-based
// biases -- is rebuilt from buildDefaultTechGraph() on load, so it is not worth persisting and
// a save cannot pin down a stale copy of a table that is still being tuned. Frontier and Next
// are pure functions of the discovered flags (TechGraph::rebuildFrontier), so they are not
// stored either. Undiscovered nodes always have science 0 (invest() only accepts Frontier
// technologies), so listing the discovered ones is complete.
static void saveTechnologies(std::ostream& out)
{
    size_t faction_count = techtree.factionCount();
    out.write(reinterpret_cast<const char*>(&faction_count), sizeof(faction_count));

    for (size_t f = 0; f < faction_count; ++f)
    {
        const TechGraph& g = techtree.graph((int)f);

        int target  = techtree.getResearchTarget((int)f);
        int pending = techtree.getPendingScience((int)f);
        out.write(reinterpret_cast<const char*>(&target), sizeof(target));
        out.write(reinterpret_cast<const char*>(&pending), sizeof(pending));

        std::vector<int> ids = g.getTechIds();
        std::vector<int> discovered;
        for (int id : ids)
            if (g.isDiscovered(id))
                discovered.push_back(id);

        size_t discovered_count = discovered.size();
        out.write(reinterpret_cast<const char*>(&discovered_count), sizeof(discovered_count));
        for (int id : discovered)
        {
            const Tech* t = g.getTech(id);
            int science = t != nullptr ? t->science : 0;
            out.write(reinterpret_cast<const char*>(&id), sizeof(id));
            out.write(reinterpret_cast<const char*>(&science), sizeof(science));
        }
    }
}

// ---------------------------------------------------------------------------------------
// Unit status and cargo.
//
// A unit's passive state (fortified / sentried / automated), WHAT improvement it is building,
// and everything aboard it if it is a Transport.
//
// Appended AFTER the technologies block as its own self-describing section -- count, then
// records that name the unit they belong to -- for the same reason the dependency and tech
// blocks are: a savegame written before this change just hits EOF here and no-ops, so every
// file already in saves/ keeps loading. Adding these fields to the unit records themselves
// would have shifted every byte after them and broken all of them (the savegame format
// carries no version header).
//
// Deliberately NOT stored: the improvement EFFORT counter. A worker three turns into a
// railroad comes back still railroading, but starting that job over from the full
// requirement for its tile -- so the only thing persisted about work is what it is doing,
// and loadUnitStatus() re-asks the improvementeffort table how long that takes. Movement
// animation state (completion, oldlatitude/oldlongitude) and a pending move are transient
// within a turn and are not stored either.

// The IMPROVEMENT_TYPES code of whatever this unit is building, or 0 if it is not working.
// One int rather than eight bools: the codes are already the on-disk vocabulary everywhere
// else, and an unknown one can simply be ignored on load.
static int unitWorkKind(Unit* u)
{
    if (u->isIrrigating())  return IRRIGATION;
    if (u->isMining())      return MINE;
    if (u->isRoading())     return ROAD;
    if (u->isRailroading()) return RAILROAD;
    if (u->isQuarrying())   return QUARRY;
    if (u->isCamping())     return CAMP;
    if (u->isDerricking())  return DERRICK;
    if (u->isPlanting())    return PLANTATION;
    return 0;
}

// Puts the unit back to work on `kind`, with a FULL effort budget for the tile it is standing
// on -- the counter is not saved, so the job restarts.
static void resumeUnitWork(Unit* u, int kind)
{
    if (kind == 0)
        return;

    int effort = getImprovementEffort(improvementeffort, kind, map.peek(u->latitude, u->longitude).bioma);

    switch (kind)
    {
        case IRRIGATION:  u->irrigating(effort);   break;
        case MINE:        u->mining(effort);       break;
        case ROAD:        u->roading(effort);      break;
        case RAILROAD:    u->railroading(effort);  break;
        case QUARRY:      u->quarrying(effort);    break;
        case CAMP:        u->camping(effort);      break;
        case DERRICK:     u->derricking(effort);   break;
        case PLANTATION:  u->planting(effort);     break;
        default:                                   break;   // unknown code: leave it idle
    }
}

static void saveUnitStatus(std::ostream& out)
{
    size_t unit_count = units.size();
    out.write(reinterpret_cast<const char*>(&unit_count), sizeof(unit_count));

    for (auto& pair : units)
    {
        Unit* u = pair.second;

        // char, not bool, so the field width on disk is unambiguous.
        char fortified = u->isFortified() ? 1 : 0;
        char sentried  = u->isSentry()    ? 1 : 0;
        char automated = u->isAuto()      ? 1 : 0;
        int  workKind  = unitWorkKind(u);

        out.write(reinterpret_cast<const char*>(&u->id), sizeof(u->id));
        out.write(&fortified, sizeof(fortified));
        out.write(&sentried,  sizeof(sentried));
        out.write(&automated, sizeof(automated));
        out.write(reinterpret_cast<const char*>(&workKind), sizeof(workKind));

        // An automated unit is one that was given a destination (goTo sets autoMode AND the
        // target), so the destination travels with the flag or the unit would come back
        // "automated" with nowhere to go.
        out.write(reinterpret_cast<const char*>(&u->target.lat), sizeof(u->target.lat));
        out.write(reinterpret_cast<const char*>(&u->target.lon), sizeof(u->target.lon));

        // Cargo. A Transport's slots hold two unrelated kinds of Shippable: boarded UNITS,
        // which live in the global units map in their own right and so are already saved
        // above (only the fact that they are aboard is missing), and RESOURCE stacks, which
        // exist nowhere else and would be lost outright. Both are written here as (kind, id,
        // amount); a passenger unit is restored by id, a resource stack is rebuilt.
        Transport* t = dynamic_cast<Transport*>(u);
        std::vector<Shippable*> cargo;
        if (t != nullptr)
            cargo = t->getCargo();

        size_t cargo_count = cargo.size();
        out.write(reinterpret_cast<const char*>(&cargo_count), sizeof(cargo_count));

        for (Shippable* s : cargo)
        {
            Unit*     passenger = dynamic_cast<Unit*>(s);
            Resource* resource  = dynamic_cast<Resource*>(s);

            char isUnit = (passenger != nullptr) ? 1 : 0;
            int  id     = s->getId();
            int  amount = (passenger == nullptr && resource != nullptr) ? resource->amount : 0;

            out.write(&isUnit, sizeof(isUnit));
            out.write(reinterpret_cast<const char*>(&id), sizeof(id));
            out.write(reinterpret_cast<const char*>(&amount), sizeof(amount));

            printf("Saving cargo of unit %d: %s %s (id %d, amount %d)\n",
                   u->id, isUnit ? "passenger" : "resource", s->getName(), id, amount);
        }
    }
}

// ---------------------------------------------------------------------------------------
// File layout.
//
//   GENERAL HEADER (plain, never hashed -- it is what tells you how to check the rest)
//       char     magic[8]         "BUNMEISV"
//       uint32   headerSize       bytes of this header, so it can grow without a new version
//       uint64   payloadSize      bytes that follow
//       uint8    digest[16]       MD5 of exactly those payloadSize bytes
//   PAYLOAD (hashed as one block)
//       SAVEGAME HEADER
//           uint32   savegameVersion      SAVEGAME_FORMAT_VERSION below
//           uint32   gameVersionLength
//           char     gameVersion[]        BUNMEI::version, e.g. "v0.0.1"
//       SAVEGAME DATA
//           year, cities, units, dependencies, technologies, unit status
//
// The payload is built in memory first so it can be hashed before a byte reaches the disk,
// which is also why every writer here takes an ostream rather than an ofstream.
//
// A load refuses outright -- no partial world -- on a bad magic, a short read, a digest
// mismatch, or a savegameVersion this build does not speak. That last one is the point of
// having a version: the format is expected to keep changing while the game is built, and a
// clean "this save is version N, I speak M" beats silently misparsing an old file into a
// corrupt world. Old saves are NOT migrated; they are throwaway test files.

#define SAVEGAME_MAGIC          "BUNMEISV"
#define SAVEGAME_MAGIC_SIZE     8
#define SAVEGAME_FORMAT_VERSION 1u

// magic + headerSize + payloadSize + digest
#define SAVEGAME_HEADER_SIZE (SAVEGAME_MAGIC_SIZE + 4 + 8 + MD5_DIGEST_SIZE)

static void writeUint32(std::ostream& out, uint32_t v)
{
    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

static void writeUint64(std::ostream& out, uint64_t v)
{
    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

static bool readUint32(std::istream& in, uint32_t& v)
{
    in.read(reinterpret_cast<char*>(&v), sizeof(v));
    return (bool)in;
}

static bool readUint64(std::istream& in, uint64_t& v)
{
    in.read(reinterpret_cast<char*>(&v), sizeof(v));
    return (bool)in;
}

// Opens a savegame, verifies it, and hands back the DATA section of its payload (everything
// after the savegame header). Returns false -- having explained why -- if the file is not a
// savegame, is truncated, fails its MD5, or is a format version this build does not speak;
// the caller must then load NOTHING, because a half-applied savegame is worse than none.
bool readSaveGame(const char* filename, std::string& data, SaveGameInfo& info)
{
    data.clear();
    info = SaveGameInfo();

    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        printf("Savegame %s: cannot open for reading.\n", filename);
        return false;
    }

    char magic[SAVEGAME_MAGIC_SIZE];
    file.read(magic, SAVEGAME_MAGIC_SIZE);
    if (!file || memcmp(magic, SAVEGAME_MAGIC, SAVEGAME_MAGIC_SIZE) != 0)
    {
        printf("Savegame %s: not a bunmei savegame (bad magic). Refusing to load.\n", filename);
        return false;
    }

    uint32_t headerSize = 0;
    uint64_t payloadSize = 0;
    if (!readUint32(file, headerSize) || !readUint64(file, payloadSize))
    {
        printf("Savegame %s: header is truncated. Refusing to load.\n", filename);
        return false;
    }

    unsigned char digest[MD5_DIGEST_SIZE];
    file.read(reinterpret_cast<char*>(digest), MD5_DIGEST_SIZE);
    if (!file)
    {
        printf("Savegame %s: header is truncated. Refusing to load.\n", filename);
        return false;
    }

    // headerSize lets the header grow later: skip anything this build does not know about
    // rather than treating it as payload. A header SMALLER than ours is a file from before
    // a field was added, which this version does not accept.
    if (headerSize < (uint32_t)SAVEGAME_HEADER_SIZE)
    {
        printf("Savegame %s: header is %u bytes, this build expects at least %d. Refusing to load.\n",
               filename, headerSize, SAVEGAME_HEADER_SIZE);
        return false;
    }
    if (headerSize > (uint32_t)SAVEGAME_HEADER_SIZE)
        file.seekg(headerSize - SAVEGAME_HEADER_SIZE, std::ios::cur);

    std::string payload;
    payload.resize((size_t)payloadSize);
    if (payloadSize > 0)
        file.read(&payload[0], (std::streamsize)payloadSize);
    if (!file || (uint64_t)file.gcount() != payloadSize)
    {
        printf("Savegame %s: payload is truncated (header says %llu bytes, got %lld). Refusing to load.\n",
               filename, (unsigned long long)payloadSize, (long long)file.gcount());
        return false;
    }
    file.close();

    unsigned char actual[MD5_DIGEST_SIZE];
    md5(payload.data(), payload.size(), actual);
    if (memcmp(actual, digest, MD5_DIGEST_SIZE) != 0)
    {
        printf("Savegame %s: INTEGRITY CHECK FAILED. Refusing to load.\n", filename);
        printf("  expected md5 %s\n", md5hex(payload.data(), payload.size()).c_str());
        char stored[MD5_DIGEST_SIZE*2 + 1];
        for (int i = 0; i < MD5_DIGEST_SIZE; i++) snprintf(stored + i*2, 3, "%02x", digest[i]);
        printf("  file says    %s\n", stored);
        return false;
    }

    // ---- savegame header, from inside the now-trusted payload ----------------------------
    std::istringstream in(payload, std::ios::binary);

    uint32_t gameVersionLength = 0;
    if (!readUint32(in, info.savegameVersion) || !readUint32(in, gameVersionLength))
    {
        printf("Savegame %s: payload header is truncated. Refusing to load.\n", filename);
        return false;
    }
    if (gameVersionLength > 0)
    {
        info.gameVersion.resize(gameVersionLength);
        in.read(&info.gameVersion[0], gameVersionLength);
        if (!in)
        {
            printf("Savegame %s: payload header is truncated. Refusing to load.\n", filename);
            return false;
        }
    }

    if (info.savegameVersion != SAVEGAME_FORMAT_VERSION)
    {
        printf("Savegame %s: format version %u, this build speaks %u (written by game %s). Refusing to load.\n",
               filename, info.savegameVersion, SAVEGAME_FORMAT_VERSION, info.gameVersion.c_str());
        return false;
    }

    // Whatever is left is the data section, already verified.
    data = payload.substr((size_t)in.tellg());

    printf("Savegame %s: format v%u, written by game %s, %zu data bytes, md5 ok.\n",
           filename, info.savegameVersion, info.gameVersion.c_str(), data.size());
    return true;
}

void savegame(const char* filename)
{
    // Every savegame (and its paired map) lives under saves/, regardless of what the
    // caller passed in -- prepend it here, once, so every caller gets this for free
    // (same "handle the directory in the function that owns the format" pattern saveMap()
    // already uses, mapio.cpp).
    std::string path = filename;
    if (path.rfind("saves/", 0) != 0)
        path = "saves/" + path;

    std::filesystem::path p(path);
    if (p.has_parent_path())
        std::filesystem::create_directories(p.parent_path());

    // Save the map (terrain, resources, ownership, improvements, per-faction visibility --
    // including mid-game changes like roads/mines/irrigation and tile ownership) alongside
    // the savegame itself, so a later -loadgame restores the exact map the game was saved
    // on (fog of war included) instead of a freshly generated one.
    saveMap(path + ".map");

    // The payload goes into memory, not straight to the file: its MD5 has to be known
    // before the header that carries it can be written.
    std::ostringstream payload(std::ios::binary);
    std::ostream& out = payload;

    // ---- savegame header: what wrote this file ------------------------------------------
    writeUint32(out, SAVEGAME_FORMAT_VERSION);
    uint32_t gameVersionLength = (uint32_t)strlen(BUNMEI::version);
    writeUint32(out, gameVersionLength);
    out.write(BUNMEI::version, gameVersionLength);

    // Save year
    out.write(reinterpret_cast<const char*>(&year), sizeof(year));

    // Save factions
    //size_t faction_count = factions.size();
    //out.write(reinterpret_cast<const char*>(&faction_count), sizeof(faction_count));
    //for (auto& f : factions) {
    //    out.write(reinterpret_cast<const char*>(&f->id), sizeof(f->id));
    //    out.write(reinterpret_cast<const char*>(f->name), sizeof(f->name));
    //    out.write(reinterpret_cast<const char*>(&f->red), sizeof(f->red));
    //    out.write(reinterpret_cast<const char*>(&f->green), sizeof(f->green));
    //    out.write(reinterpret_cast<const char*>(&f->blue), sizeof(f->blue));
    //    out.write(reinterpret_cast<const char*>(f->rates), sizeof(f->rates));
    //    out.write(reinterpret_cast<const char*>(&f->autoPlayer), sizeof(f->autoPlayer));
    // }


    // Save cities
    size_t city_count = cities.size();
    out.write(reinterpret_cast<const char*>(&city_count), sizeof(city_count));
    for (auto& pair : cities) {
        City* c = pair.second;
        out.write(reinterpret_cast<const char*>(&c->id), sizeof(c->id));
        bool isCapital = c->isCapitalCity();
        out.write(reinterpret_cast<const char*>(&isCapital), sizeof(isCapital));
        out.write(reinterpret_cast<const char*>(&c->faction), sizeof(c->faction));
        out.write(reinterpret_cast<const char*>(&c->latitude), sizeof(c->latitude));
        out.write(reinterpret_cast<const char*>(&c->longitude), sizeof(c->longitude));
        out.write(reinterpret_cast<const char*>(&c->pop), sizeof(c->pop));
        out.write(reinterpret_cast<const char*>(&c->shields), sizeof(c->shields));
        out.write(reinterpret_cast<const char*>(&c->food), sizeof(c->food));
        out.write(reinterpret_cast<const char*>(&c->foundedyear), sizeof(c->foundedyear));

        // Save city name
        size_t name_len = strlen(c->name);
        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(c->name, name_len);

        // The buildings are missing.

        // Save resources
        size_t resource_count = c->resources.size();
        out.write(reinterpret_cast<const char*>(&resource_count), sizeof(resource_count));
        for (const auto& res : c->resources) {
            out.write(reinterpret_cast<const char*>(&res.first), sizeof(res.first));
            out.write(reinterpret_cast<const char*>(&res.second), sizeof(res.second));
        }

        // Save working tiles
        int number_of_working_tiles = c->numberOfWorkingTiles();
        out.write(reinterpret_cast<const char*>(&number_of_working_tiles), sizeof(number_of_working_tiles));
        // Only the tile's offset RELATIVE to the city (-3..3) is stored, so no absolute
        // coordinate is needed here at all -- loadCities reads the pairs back the same way.
        for(int lats=-3;lats<=3;lats++)
            for(int lons=-3;lons<=3;lons++)
            {
                if (c->workingOn(lats,lons))
                {
                    printf("Saving working tile at offset (%d,%d) for city %s\n", lats, lons, c->name);
                    out.write(reinterpret_cast<const char*>(&lats), sizeof(lats));
                    out.write(reinterpret_cast<const char*>(&lons), sizeof(lons));
                }
            }



        // Save buildings
        size_t building_count = c->buildings.size();
        out.write(reinterpret_cast<const char*>(&building_count), sizeof(building_count));
        for (const auto& b : c->buildings) {
            int buildingType = b->getSubType(); // Assumes Building has getType()
            out.write(reinterpret_cast<const char*>(&buildingType), sizeof(buildingType));
        }

        // // Save buildable
        // size_t buildable_count = c->buildable.size();
        // out.write(reinterpret_cast<const char*>(&buildable_count), sizeof(buildable_count));
        // for (const auto& bf : c->buildable) {
        //     int buildableType = bf->getType(); // Assumes BuildableFactory has getType()
        //     out.write(reinterpret_cast<const char*>(&buildableType), sizeof(buildableType));
        // }


        // // Save productionQueue
        // size_t queue_count = c->productionQueue.size();
        // out.write(reinterpret_cast<const char*>(&queue_count), sizeof(queue_count));
        // std::queue<BuildableFactory*> tempQueue = c->productionQueue;
        // while (!tempQueue.empty()) {
        //     BuildableFactory* bf = tempQueue.front();
        //     int buildableType = bf->getType(); // Assumes BuildableFactory has getType()
        //     out.write(reinterpret_cast<const char*>(&buildableType), sizeof(buildableType));
        //     tempQueue.pop();
        // }




    }

    // Save units
    size_t unit_count = units.size();
    out.write(reinterpret_cast<const char*>(&unit_count), sizeof(unit_count));
    for (auto& pair : units) {
        Unit* u = pair.second;
        int subType = u->getSubType();
        printf("Saving unit ID %d of subtype %d\n", u->id, subType);
        out.write(reinterpret_cast<const char*>(&subType), sizeof(subType));
        out.write(reinterpret_cast<const char*>(&u->id), sizeof(u->id));
        out.write(reinterpret_cast<const char*>(&u->faction), sizeof(u->faction));
        out.write(reinterpret_cast<const char*>(&u->latitude), sizeof(u->latitude));
        out.write(reinterpret_cast<const char*>(&u->longitude), sizeof(u->longitude));
        out.write(reinterpret_cast<const char*>(&u->availablemoves), sizeof(u->availablemoves));
        // Save unit name if present
        size_t name_len = strlen(u->name);
        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(u->name, name_len);
    }

    saveDependencies(out);
    saveTechnologies(out);
    saveUnitStatus(out);

    // ---- general header, then the payload it describes -----------------------------------
    const std::string bytes = payload.str();

    unsigned char digest[MD5_DIGEST_SIZE];
    md5(bytes.data(), bytes.size(), digest);

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        printf("Error opening %s for writing.\n", path.c_str());
        return;
    }

    file.write(SAVEGAME_MAGIC, SAVEGAME_MAGIC_SIZE);
    writeUint32(file, (uint32_t)SAVEGAME_HEADER_SIZE);
    writeUint64(file, (uint64_t)bytes.size());
    file.write(reinterpret_cast<const char*>(digest), MD5_DIGEST_SIZE);
    file.write(bytes.data(), (std::streamsize)bytes.size());
    file.close();

    printf("Game saved to %s (savegame format v%u, game %s, %zu payload bytes, md5 %s)\n",
           path.c_str(), SAVEGAME_FORMAT_VERSION, BUNMEI::version, bytes.size(),
           md5hex(bytes.data(), bytes.size()).c_str());
}


void loadCities(std::istream& in)
{
    // Load cities
    size_t city_count = 0;
    in.read(reinterpret_cast<char*>(&city_count), sizeof(city_count));
    printf("Loading %zu cities...\n", city_count);
    for (size_t i = 0; i < city_count; ++i) {
        // City's constructor unconditionally claims map tile (0,0) -- the map's actual
        // origin, not a safe placeholder -- as a side effect of these temporary (0,0,0,0)
        // values (real id/latitude/longitude are read right after and may not even BE
        // (0,0)). loadMap() (initMap(), called before loadWorldModelling() reaches this
        // function) already restored that tile's correct ownership from the map file;
        // snapshot/restore it around construction so this placeholder claim doesn't corrupt
        // whatever legitimately occupies it (including this very city, if it turns out to
        // actually be located at (0,0)).
        // peek(), NOT map(0,0): operator() adds the viewing faction's map offset while the
        // restore below goes through set(), which does not -- so with the view scrolled this
        // snapshotted one tile and wrote it over a DIFFERENT one. Latent today (the offset is
        // still 0 this early in loadWorldModelling), but the two sides must match.
        mapcell origin_snapshot = map.peek(0,0);
        City* c = new City(&map, 0, 0, 0, 0); // Temporary values
        map.set(0,0) = origin_snapshot;
        in.read(reinterpret_cast<char*>(&c->id), sizeof(c->id));
        bool isCapital;
        in.read(reinterpret_cast<char*>(&isCapital), sizeof(isCapital));
        // The flag that was just read decides this -- calling setCapitalCity() unconditionally
        // made EVERY loaded city a capital, which is not cosmetic: endOfYear() charges the
        // faction's whole salary bill to each capital (bunmei.cpp), so a four-city faction
        // paid four times over, and factionTreasury() picked an arbitrary one. The City
        // constructor already clears the flag, so there is nothing to do in the false case.
        if (isCapital)
            c->setCapitalCity();
        in.read(reinterpret_cast<char*>(&c->faction), sizeof(c->faction));
        in.read(reinterpret_cast<char*>(&c->latitude), sizeof(c->latitude));
        in.read(reinterpret_cast<char*>(&c->longitude), sizeof(c->longitude));
        in.read(reinterpret_cast<char*>(&c->pop), sizeof(c->pop));
        in.read(reinterpret_cast<char*>(&c->shields), sizeof(c->shields));
        in.read(reinterpret_cast<char*>(&c->food), sizeof(c->food));
        in.read(reinterpret_cast<char*>(&c->foundedyear), sizeof(c->foundedyear));

        // Load city name
        size_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        in.read(c->name, name_len);
        c->name[name_len] = '\0'; // Null-terminate the string

        printf("Loaded city: %s (ID: %d, Faction: %d, Location: (%d, %d), Pop: %d)\n",
               c->name, c->id, c->faction, c->latitude, c->longitude, c->pop);

        cities[c->id] = c;
        citynames[c->faction].pop(); // Remove the used name

        // Load resources
        size_t resource_count = 0;
        in.read(reinterpret_cast<char*>(&resource_count), sizeof(resource_count));

        for (size_t j = 0; j < resource_count; ++j) {
            int res_id = 0, res_amount = 0;
            in.read(reinterpret_cast<char*>(&res_id), sizeof(res_id));
            in.read(reinterpret_cast<char*>(&res_amount), sizeof(res_amount));
            c->resources[res_id] = res_amount;
        }

        // Working tiles are NOT re-assigned here: the map file (saveMap()/loadMap(),
        // mapio.cpp) already serializes each tile's c_id_owner/f_id_owner/owners, and
        // initMap() loads that map BEFORE loadCities() runs (see the @NOTE above), so a
        // city's working tiles are already correctly restored by the time we get here.
        // City::assignWorkingTile(coordinate) is a TOGGLE (used by the city UI's tile
        // click) -- calling it on a tile the map file already assigned to this city would
        // RELEASE it instead of leaving it alone, silently losing every working tile except
        // the city centre (which reSetCities()'s own workaround patches back in every tick
        // regardless). Still read the (lats,lons) pairs, purely to stay aligned with what
        // savegame() wrote (buildings and the next city/unit follow in the stream).
        int number_of_working_tiles = 0;
        in.read(reinterpret_cast<char*>(&number_of_working_tiles), sizeof(number_of_working_tiles));
        for (int j = 0; j < number_of_working_tiles; ++j) {
            int lats = 0;
            int lons = 0;
            in.read(reinterpret_cast<char*>(&lats), sizeof(lats));
            in.read(reinterpret_cast<char*>(&lons), sizeof(lons));
            printf("City %s already working tile at offset (%d,%d) (restored from the map file)\n", c->name, lats, lons);
        }

        // Load buildings
        size_t building_count = 0;
        in.read(reinterpret_cast<char*>(&building_count), sizeof(building_count));
        for (size_t j = 0; j < building_count; ++j) {
            int buildingType = 0;
            in.read(reinterpret_cast<char*>(&buildingType), sizeof(buildingType));
            Building* building = nullptr;
            switch (buildingType) {
                case BUILDING_PALACE:
                building = new Palace();
                break;
                case BUILDING_BARRACKS:
                building = new Barracks();
                break;
                case BUILDING_GRANARY:
                building = new Granary();
                break;
                case BUILDING_COLLOSSEUM:
                building = new Collosseum();
                break;
                default:
                building = new Palace(); // Default to Palace if unknown
                break;
            }

            c->buildings.push_back(building);
        }

        City* city = c;
        city->buildable.push_back(new BarracksFactory());
        city->buildable.push_back(new PalaceFactory());
        city->buildable.push_back(new SettlerFactory());
        city->buildable.push_back(new GranaryFactory());
        city->buildable.push_back(new CollosseumFactory());
        city->buildable.push_back(new WarriorFactory());
        city->buildable.push_back(new ArcherFactory());
        city->buildable.push_back(new SpearmanFactory());
        city->buildable.push_back(new SwordmanFactory());
        city->buildable.push_back(new AxemanFactory());
        city->buildable.push_back(new WorkerFactory());
        city->buildable.push_back(new HorsemanFactory());
        city->buildable.push_back(new TriremeFactory());
        city->buildable.push_back(new GalleyFactory());
        city->buildable.push_back(new HorsearcherFactory());
    }    
}


void loadUnits(std::istream& in)
{
    // Load units
    size_t unit_count = 0;
    in.read(reinterpret_cast<char*>(&unit_count), sizeof(unit_count));

    printf("Loading %zu units...\n", unit_count);
    for (size_t i = 0; i < unit_count; ++i) {
        Unit* u = nullptr;

        int subType = 0;
        in.read(reinterpret_cast<char*>(&subType), sizeof(subType));

        printf("Loading unit of subtype %d...\n", subType);

        switch (subType) {
            case UNIT_ARCHER:
            u = new Archer(); // Replace with Archer class if available
            break;
            case UNIT_WARRIOR:
            u = new Warrior();
            break;
            case UNIT_SPEARMAN:
            u = new Spearman(); // Replace with Spearman class if available
            break;
            case UNIT_SWORDMAN:
            u = new Swordman(); // Replace with Swordman class if available
            break;
            case UNIT_AXEMAN:
            u = new Axeman(); // Replace with Axeman class if available
            break;
            case UNIT_HORSEMAN:
            u = new Horseman(); // Replace with Horseman class if available
            break;
            case UNIT_HORSEARCHER:
            u = new Horsearcher(); // Replace with HorseArcher class if available
            break;
            case UNIT_SETTLER:
            u = new Settler();
            break;
            case UNIT_WORKER:
            u = new Worker(); // Replace with Worker class if available
            break;
            case UNIT_TRIREME:
            u = new Trireme();
            break;
            case UNIT_GALLEON:
            u = new Galleon();
            break;
            case UNIT_GALLEY:
            u = new Galley(); // Replace with Galley class if available
            break;
            case UNIT_CHARIOT:
            u = new Chariot(); // Replace with Chariot class if available
            break;
            default:
            u = new Warrior();
            break;
        }

        in.read(reinterpret_cast<char*>(&u->id), sizeof(u->id));
        in.read(reinterpret_cast<char*>(&u->faction), sizeof(u->faction));
        in.read(reinterpret_cast<char*>(&u->latitude), sizeof(u->latitude));
        in.read(reinterpret_cast<char*>(&u->longitude), sizeof(u->longitude));
        in.read(reinterpret_cast<char*>(&u->availablemoves), sizeof(u->availablemoves));

        // Load unit name
        size_t name_len = 0;
        char str[256];
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        in.read(str, name_len);
        strncpy(u->name, str, name_len);
        u->name[name_len] = '\0';

        printf("Loaded unit: %s (ID: %d, Faction: %d, Location: (%d, %d), Available Moves: %.2f)\n",
               u->name, u->id, u->faction, u->latitude, u->longitude, u->availablemoves);

        units[u->id] = u;
    }    
}

void loadgame()
{
    std::ifstream in("savegame.dat", std::ios::binary);
    if (!in) {
        printf("Error opening savegame.dat for reading.\n");
        return;
    }

    // Load factions
    size_t faction_count = 0;
    in.read(reinterpret_cast<char*>(&faction_count), sizeof(faction_count));
    for (size_t i = 0; i < faction_count; ++i) {
        Faction* f = new Faction();
        in.read(reinterpret_cast<char*>(&f->id), sizeof(f->id));
        in.read(reinterpret_cast<char*>(f->name), sizeof(f->name));
        in.read(reinterpret_cast<char*>(&f->red), sizeof(f->red));
        in.read(reinterpret_cast<char*>(&f->green), sizeof(f->green));
        in.read(reinterpret_cast<char*>(&f->blue), sizeof(f->blue));
        in.read(reinterpret_cast<char*>(f->rates), sizeof(f->rates));
        in.read(reinterpret_cast<char*>(&f->autoPlayer), sizeof(f->autoPlayer));
        factions.push_back(f);
    }





}



// Replaces the whole registry with the saved one -- authoritative, so anything registered
// during setup (initTechnologies' root code) or while loading is superseded. This is also what
// restores CITY-level perks: loadCities() rebuilds a city's buildings but never re-registers
// their perk codes (only bunmei.cpp does, when one is actually built), so before this they
// were silently lost across a save/load.
void loadDependencies(std::istream& in)
{
    size_t context_count = 0;
    in.read(reinterpret_cast<char*>(&context_count), sizeof(context_count));
    if (!in) return;

    dee.clear();

    for (size_t i = 0; i < context_count; ++i)
    {
        int contextId = 0;
        size_t code_count = 0;
        in.read(reinterpret_cast<char*>(&contextId), sizeof(contextId));
        in.read(reinterpret_cast<char*>(&code_count), sizeof(code_count));
        if (!in) return;

        for (size_t j = 0; j < code_count; ++j)
        {
            int codeId = 0;
            in.read(reinterpret_cast<char*>(&codeId), sizeof(codeId));
            if (!in) return;
            dee.regDep(contextId, codeId);
        }
    }

    printf("Loaded dependencies for %zu contexts.\n", context_count);
}

// Rebuilds each faction's progress on top of the graph initTechnologies() already created.
// A technology code that no longer exists (the table changed since the save) is skipped rather
// than treated as an error -- that is the whole point of storing codes instead of indices.
void loadTechnologies(std::istream& in)
{
    size_t faction_count = 0;
    in.read(reinterpret_cast<char*>(&faction_count), sizeof(faction_count));
    if (!in) return;

    for (size_t f = 0; f < faction_count; ++f)
    {
        int target = 0, pending = 0;
        size_t discovered_count = 0;
        in.read(reinterpret_cast<char*>(&target), sizeof(target));
        in.read(reinterpret_cast<char*>(&pending), sizeof(pending));
        in.read(reinterpret_cast<char*>(&discovered_count), sizeof(discovered_count));
        if (!in) return;

        bool known = (int)f < techtree.factionCount();
        TechGraph* g = known ? &techtree.graph((int)f) : nullptr;

        int skipped = 0;
        for (size_t i = 0; i < discovered_count; ++i)
        {
            int id = 0, science = 0;
            in.read(reinterpret_cast<char*>(&id), sizeof(id));
            in.read(reinterpret_cast<char*>(&science), sizeof(science));
            if (!in) return;

            Tech* t = g != nullptr ? g->getTech(id) : nullptr;
            if (t == nullptr) { skipped++; continue; }

            t->discovered = true;
            t->science    = science;
        }

        if (g == nullptr)
            continue;

        // Everything derived comes back from the flags above.
        g->rebuildFrontier();

        // setResearchTarget only accepts a technology that is in the rebuilt Frontier, so this
        // silently drops a target that no longer makes sense; endOfYear's chooseResearch() then
        // asks for a new one.
        techtree.setResearchTarget((int)f, target);
        techtree.setPendingScience((int)f, pending);

        if (skipped > 0)
            printf("Faction %zu: skipped %d saved technologies that no longer exist.\n", f, skipped);
    }

    printf("Loaded technologies for %zu factions.\n", faction_count);
}

// Restores what saveUnitStatus() wrote. Reaching EOF immediately is not an error: that is
// simply a savegame written before this block existed, and every unit keeps the idle,
// empty-handed state loadUnits() left it in.
void loadUnitStatus(std::istream& in)
{
    size_t unit_count = 0;
    in.read(reinterpret_cast<char*>(&unit_count), sizeof(unit_count));
    if (!in)
    {
        printf("No unit status block in this savegame (written before it existed) -- units load idle.\n");
        return;
    }

    // Boarding is resolved in a second pass: a passenger's own unit record may sit anywhere
    // in the file, and a Transport can itself be cargo, so nothing may be boarded until every
    // unit is known to exist.
    struct PendingCargo { int transportId; bool isUnit; int id; int amount; };
    std::vector<PendingCargo> pending;

    int restored = 0;
    for (size_t i = 0; i < unit_count; ++i)
    {
        int  unitId = 0;
        char fortified = 0, sentried = 0, automated = 0;
        int  workKind = 0;
        int  targetLat = 0, targetLon = 0;
        size_t cargo_count = 0;

        in.read(reinterpret_cast<char*>(&unitId), sizeof(unitId));
        in.read(&fortified, sizeof(fortified));
        in.read(&sentried,  sizeof(sentried));
        in.read(&automated, sizeof(automated));
        in.read(reinterpret_cast<char*>(&workKind), sizeof(workKind));
        in.read(reinterpret_cast<char*>(&targetLat), sizeof(targetLat));
        in.read(reinterpret_cast<char*>(&targetLon), sizeof(targetLon));
        in.read(reinterpret_cast<char*>(&cargo_count), sizeof(cargo_count));
        if (!in) return;

        // find(), never units[id]: operator[] on a missing key inserts a null that the next
        // dereference walks into. A record naming a unit that no longer loads is skipped,
        // but its cargo entries still have to be READ to stay aligned in the stream.
        auto it = units.find(unitId);
        Unit* u = (it != units.end()) ? it->second : nullptr;

        if (u != nullptr)
        {
            if (fortified) u->fortify();
            if (sentried)  u->sentry();
            if (automated) u->goTo(targetLat, targetLon);   // sets autoMode and the target
            resumeUnitWork(u, workKind);
            restored++;
        }

        for (size_t c = 0; c < cargo_count; ++c)
        {
            char isUnit = 0;
            int  id = 0, amount = 0;
            in.read(&isUnit, sizeof(isUnit));
            in.read(reinterpret_cast<char*>(&id), sizeof(id));
            in.read(reinterpret_cast<char*>(&amount), sizeof(amount));
            if (!in) return;

            if (u != nullptr)
                pending.push_back({ unitId, isUnit != 0, id, amount });
        }
    }

    int boarded = 0;
    for (const PendingCargo& pc : pending)
    {
        auto tit = units.find(pc.transportId);
        Transport* t = (tit != units.end()) ? dynamic_cast<Transport*>(tit->second) : nullptr;
        if (t == nullptr)
            continue;                       // the unit came back as something that cannot carry

        if (pc.isUnit)
        {
            auto pit = units.find(pc.id);
            if (pit == units.end())
                continue;

            // dynamic_cast, never a C-style cast: Unit does not inherit Shippable, so only a
            // cross-cast adjusts for the subobject offsets (see moveOntoNavalUnit). It can
            // legitimately fail -- a Trireme/Galley/Wagon is not Shippable.
            Shippable* passenger = dynamic_cast<Shippable*>(pit->second);
            if (passenger != nullptr && t->board(passenger))
            {
                boarded++;
                printf("Re-boarded unit %d onto transport %d.\n", pc.id, pc.transportId);
            }
        }
        else
        {
            // Resource cargo exists nowhere but aboard, so it is rebuilt here, the same way
            // engine.cpp's LoadCargoOrder builds it.
            bool ismfggood = pc.id >= rum;   // MFGOODS start at 0x301 (rum), COMMODITIES at 0x201.
            Resource* cargoitem = ismfggood
                ? (Resource*) new MfgGood(pc.id, tiles[pc.id].c_str(), "MfgGood")
                : (Resource*) new Commodity(pc.id, tiles[pc.id].c_str(), "Commodity");
            cargoitem->amount = pc.amount;

            Shippable* s = dynamic_cast<Shippable*>(cargoitem);
            if (s != nullptr && t->board(s))
            {
                boarded++;
                printf("Reloaded %d of resource 0x%x onto transport %d.\n", pc.amount, pc.id, pc.transportId);
            }
            else
            {
                delete cargoitem;            // no room aboard: do not leak the stack
            }
        }
    }

    printf("Loaded status for %d units, %d cargo item(s) re-boarded.\n", restored, boarded);
}
