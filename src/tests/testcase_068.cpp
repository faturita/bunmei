//  TestCase_068.cpp
//  bunmei
//
//  Created by Claude on 21/09/2026
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../improvements.h"
#include "../usercontrols.h"
#include "../mapio.h"
#include "../savegame.h"
#include "../md5.h"
#include "../version.h"

#include "testcase_068.h"

// @Task: a savegame now carries a general header (magic + payload length + MD5) wrapping a
// payload that itself starts with a version header (savegame format version + the
// BUNMEI::version that wrote it), and a load is REFUSED outright if the integrity or the
// version does not check out.
//
// Two halves:
//   A. MD5 itself, against RFC 1321's own test vectors. This matters more than it looks: the
//      digest is only ever compared against one this same code produced, so a subtly wrong
//      implementation would pass every round-trip test in the suite and still not be MD5.
//      The 62- and 80-byte vectors are the ones that exercise the two-block padding tail.
//   B. the file format, by corrupting a REAL savegame in each of the ways that can happen
//      and checking each one is refused, with the good file still accepted in between.

extern Map map;
extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern float mapzoom;

extern Coordinator coordinator;
extern int year;

#define TEST_MAPSIZE 1

// Offsets into the general header, which is magic[8] + headerSize(4) + payloadSize(8) + md5(16).
#define OFF_MAGIC         0
#define OFF_HEADER_SIZE   8
#define OFF_PAYLOAD_SIZE  12
#define OFF_DIGEST        20
#define HEADER_BYTES      36

TestCase_068::TestCase_068() {}
TestCase_068::~TestCase_068() {}

int TestCase_068::number() { return 68; }

void TestCase_068::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);
    initImprovements(improvements);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(LAND);
            cell.bioma = GRASSLAND;
            cell.setVisible(0);
        }
    assignProductionRates(map);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    citynames[0] = std::queue<std::string>();

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->setCapitalCity();
    cities[city->id] = city;
    citynames[0].push("Kattegate");

    Warrior* w = new Warrior();
    w->id = getNextUnitId();
    w->faction = 0;
    w->latitude = 1; w->longitude = 1;
    w->availablemoves = w->getUnitMoves();
    w->fortify();
    units[w->id] = w;

    char namebuf[64];
    snprintf(namebuf, sizeof(namebuf), "testcase068_%d", (int)((time(nullptr) ^ getpid()) & 0xffffff));
    savename = std::string("saves/") + namebuf;
    savegame(savename.c_str());

    // The GLOBAL year is what savegame() wrote; check()'s parameter shadows it with the
    // tester's tick-year, so capture it here.
    savedYear = ::year;

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_068::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 5) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    // ================= A. MD5 against RFC 1321's test vectors ===========================
    {
        struct Vector { const char* in; const char* want; };
        const Vector vectors[] = {
            { "",                                                               "d41d8cd98f00b204e9800998ecf8427e" },
            { "a",                                                              "0cc175b9c0f1b6a831c399e269772661" },
            { "abc",                                                            "900150983cd24fb0d6963f7d28e17f72" },
            { "message digest",                                                 "f96b697d7cb7938d525a2f31aaf161d0" },
            { "abcdefghijklmnopqrstuvwxyz",                                     "c3fcd3d76192e4007dfb496cca67e13b" },
            // 62 bytes: the padding needs a second block.
            { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f" },
            // 80 bytes: an exact multiple of the 64-byte block plus a full tail block.
            { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
                                                                                "57edf4a22be3c955ac49da2e2107b67a" }
        };

        for (const Vector& v : vectors)
        {
            std::string got = md5hex(v.in, strlen(v.in));
            if (got != v.want)
            {
                char buf[240];
                snprintf(buf,sizeof(buf),"md5(\"%.20s\") = %s, RFC 1321 says %s.", v.in, got.c_str(), v.want);
                fail(buf); return 0;
            }
        }
    }

    // ================= B. the savegame header and its checks ============================
    // Read the file this test's init() wrote, once, as bytes to be corrupted in various ways.
    std::string good;
    {
        std::ifstream f(savename, std::ios::binary);
        if (!f) { fail("The savegame this test wrote cannot be opened."); return 0; }
        std::ostringstream all;
        all << f.rdbuf();
        good = all.str();
    }

    if (good.size() < HEADER_BYTES + 8)
    { fail("The savegame is too small to even hold a header -- savegame() did not write it."); return 0; }

    // The magic and the declared header size, in the file, where the loader expects them.
    if (memcmp(good.data() + OFF_MAGIC, "BUNMEISV", 8) != 0)
    { fail("The savegame does not start with the BUNMEISV magic."); return 0; }

    uint32_t headerSize = 0;
    uint64_t payloadSize = 0;
    memcpy(&headerSize,  good.data() + OFF_HEADER_SIZE,  sizeof(headerSize));
    memcpy(&payloadSize, good.data() + OFF_PAYLOAD_SIZE, sizeof(payloadSize));

    if (headerSize != HEADER_BYTES)
    {
        char buf[160];
        snprintf(buf,sizeof(buf),"The header declares %u bytes; the layout is %d.", headerSize, HEADER_BYTES);
        fail(buf); return 0;
    }
    if (payloadSize != good.size() - HEADER_BYTES)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"The header declares a %llu-byte payload but %zu bytes follow it.",
                 (unsigned long long)payloadSize, good.size() - HEADER_BYTES);
        fail(buf); return 0;
    }

    // The stored digest really is the MD5 of the payload that follows it.
    unsigned char stored[MD5_DIGEST_SIZE];
    memcpy(stored, good.data() + OFF_DIGEST, MD5_DIGEST_SIZE);
    unsigned char actual[MD5_DIGEST_SIZE];
    md5(good.data() + HEADER_BYTES, good.size() - HEADER_BYTES, actual);
    if (memcmp(stored, actual, MD5_DIGEST_SIZE) != 0)
    { fail("The digest in the header is not the MD5 of the payload."); return 0; }

    // Writes `bytes` to a scratch file and reports whether readSaveGame() accepts it.
    std::string probe = savename + ".probe";
    auto accepted = [&](const std::string& bytes)->bool
    {
        {
            std::ofstream f(probe, std::ios::binary);
            if (!f) return false;
            f.write(bytes.data(), (std::streamsize)bytes.size());
        }
        std::string data;
        SaveGameInfo info;
        return readSaveGame(probe.c_str(), data, info);
    };

    // ---- the untouched file is accepted, and says what wrote it --------------------------
    {
        std::string data;
        SaveGameInfo info;
        if (!readSaveGame(savename.c_str(), data, info))
        { fail("readSaveGame() rejected the intact savegame this test just wrote."); return 0; }

        if (info.savegameVersion == 0)
        { fail("The savegame header carries no format version."); return 0; }
        if (info.gameVersion != std::string(BUNMEI::version))
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"The savegame says it was written by game '%s', this build is '%s'.",
                     info.gameVersion.c_str(), BUNMEI::version);
            fail(buf); return 0;
        }
        // The data handed back is the payload MINUS its own header, and starts with the year.
        if (data.size() >= payloadSize)
        { fail("readSaveGame() returned the whole payload; the savegame header should be consumed."); return 0; }
        int firstField = 0;
        memcpy(&firstField, data.data(), sizeof(firstField));
        if (firstField != savedYear)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"The data section starts with %d, expected the saved year %d.", firstField, savedYear);
            fail(buf); return 0;
        }
    }

    // ---- every way it can be broken is refused -------------------------------------------
    struct Corruption { const char* what; std::string bytes; };
    std::vector<Corruption> corruptions;

    // A byte flipped in the middle of the payload: the length still matches, only the MD5
    // catches this -- it is the case the hash exists for.
    {
        std::string b = good;
        b[HEADER_BYTES + (b.size() - HEADER_BYTES)/2] ^= 0xff;
        corruptions.push_back({ "a flipped byte inside the payload", b });
    }
    // A byte flipped in the stored digest.
    {
        std::string b = good;
        b[OFF_DIGEST] ^= 0x01;
        corruptions.push_back({ "a flipped byte in the stored digest", b });
    }
    // Truncated payload -- a half-written save, the most likely real accident.
    {
        std::string b = good.substr(0, good.size() - 16);
        corruptions.push_back({ "a truncated payload", b });
    }
    // Truncated inside the header itself.
    corruptions.push_back({ "a truncated header", good.substr(0, HEADER_BYTES - 4) });
    // Not a savegame at all.
    corruptions.push_back({ "a file that is not a savegame", std::string("not a savegame at all, just text") });
    // An empty file.
    corruptions.push_back({ "an empty file", std::string() });
    // A format version this build does not speak: the first field of the payload.
    {
        std::string b = good;
        uint32_t bogus = 999u;
        memcpy(&b[HEADER_BYTES], &bogus, sizeof(bogus));
        // ...and re-hash, so the ONLY thing wrong is the version. Otherwise the MD5 check
        // would reject it first and the version check would never be reached.
        unsigned char d[MD5_DIGEST_SIZE];
        md5(b.data() + HEADER_BYTES, b.size() - HEADER_BYTES, d);
        memcpy(&b[OFF_DIGEST], d, MD5_DIGEST_SIZE);
        corruptions.push_back({ "an unknown savegame format version (correctly hashed)", b });
    }

    for (const Corruption& c : corruptions)
    {
        printf("--- expecting a refusal for: %s\n", c.what);
        if (accepted(c.bytes))
        {
            char buf[240];
            snprintf(buf,sizeof(buf),"readSaveGame() ACCEPTED a savegame with %s -- it must refuse.", c.what);
            fail(buf); return 0;
        }
    }

    // ---- and the good file is still accepted after all that ------------------------------
    if (!accepted(good))
    { fail("readSaveGame() rejected the intact savegame after the corruption probes."); return 0; }

    haspassed = true;
    return 0;
}

std::string TestCase_068::title()
{
    return std::string("Savegame integrity: a general header (BUNMEISV magic + payload length + MD5) wraps a payload whose own header carries the savegame format version and the BUNMEI::version that wrote it; readSaveGame() refuses a bad magic, a truncated header or payload, a flipped byte in either the payload or the digest, and an unknown format version. Includes MD5 known-answer tests from RFC 1321.");
}

bool TestCase_068::done()   { return isdone; }
bool TestCase_068::passed() { return haspassed; }
std::string TestCase_068::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_068();
}
