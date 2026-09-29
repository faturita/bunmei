//  TestCase_083.cpp
//  bunmei
//
//  Created by faturita on 29/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "../map.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../diplomacy.h"
#include "../dee.h"
#include "../technologies.h"

#include "testcase_083.h"

// CommandOrders are serialized, so a whole match can be replayed from its log (and, later,
// sent over the network). Each command gets an incremental id and the year it was issued
// (Coordinator::push); serialize()/deserialize() turn it into one text line and back, and
// every command processCommandOrders() pops is appended to `matchlog`.

extern Map map;
extern Factions factions;
extern Tiles tiles;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;
extern DiplomacyTable diplomacy;
extern char matchlog[256];
extern int year;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_083::TestCase_083()
{

}

TestCase_083::~TestCase_083()
{

}

int TestCase_083::number()
{
    return 83;
}

// Two commands are the same command: every field, compared directly (rates bit for bit).
static bool sameCommand(const CommandOrder& a, const CommandOrder& b)
{
    const commandparameters& p = a.parameters;
    const commandparameters& q = b.parameters;
    for (int i = 0; i < FUNDAMENTAL_RATES; i++)
        if (p.rates[i] != q.rates[i])
            return false;
    return a.id == b.id && a.year == b.year && a.command == b.command &&
           p.spawnid == q.spawnid && p.factionid == q.factionid && p.cityid == q.cityid &&
           p.latitude == q.latitude && p.longitude == q.longitude &&
           p.selectedbuildableid == q.selectedbuildableid && p.resourceid == q.resourceid &&
           p.targetfactionid == q.targetfactionid && p.status == q.status && p.techid == q.techid &&
           p.enabled == q.enabled && p.scope == q.scope && p.codeid == q.codeid;
}

void TestCase_083::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
            map.set(lat,lon).setVisible(0);
        }

    Faction *faction = createFaction(0);
    faction->autoPlayer = false;        // a human with no units: nothing is pushed on its own
    factions.push_back(faction);

    initDiplomacy(diplomacy, (int)factions.size());
    initTechnologies(techtree, (int)factions.size(), dee);

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

    // Empty the log a previous run left behind (no command has run yet, so the recorder has
    // not opened it): otherwise its lines would pass for this run's.
    FILE* stale = fopen("tmp/match.log", "w");
    if (stale != nullptr)
        fclose(stale);

}

int TestCase_083::check(int checkyear)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    isdone = true;
    haspassed = false;
    char buf[400];

    // ---- round trip: every field set, negative and fractional values ---------------------
    CommandOrder full;
    full.id = 123456;
    full.year = -3999;
    full.command = Command::SetDiplomacyOrder;
    full.parameters.spawnid = 42;
    full.parameters.factionid = 3;
    full.parameters.cityid = 7;
    full.parameters.latitude = -12;
    full.parameters.longitude = -35;
    full.parameters.selectedbuildableid = 11;
    full.parameters.resourceid = 205;
    full.parameters.rates[0] = 0.1f; full.parameters.rates[1] = 0.2f;
    full.parameters.rates[2] = 0.3f; full.parameters.rates[3] = 1.0f / 3.0f;
    full.parameters.targetfactionid = 9;
    full.parameters.status = 4;
    full.parameters.techid = 0x2e;
    full.parameters.enabled = true;
    full.parameters.scope = 2;
    full.parameters.codeid = 99;

    std::string line = full.serialize();
    CommandOrder back;
    if (!CommandOrder::deserialize(line, back) || !sameCommand(full, back))
    {
        snprintf(buf, sizeof(buf), "round trip lost something: '%s' -> '%s'.", line.c_str(), back.serialize().c_str());
        message = std::string(buf);
        return 0;
    }
    if (line.find('\n') != std::string::npos)
    {
        message = std::string("serialize() must give ONE line without its newline.");
        return 0;
    }

    // ---- the text itself: zero fields are left out ----------------------------------------
    CommandOrder build;
    build.id = 17; build.year = -3999;
    build.command = Command::BuildCityOrder;
    build.parameters.spawnid = 42;          // factionid 0, everything else 0
    if (build.serialize() != "17 -3999 1 u=42")
    {
        snprintf(buf, sizeof(buf), "BuildCityOrder for unit 42 serialized as '%s', expected '17 -3999 1 u=42'.", build.serialize().c_str());
        message = std::string(buf);
        return 0;
    }
    CommandOrder none;
    if (!CommandOrder::deserialize(none.serialize(), back) || !sameCommand(none, back) || none.serialize() != "0 0 0")
    {
        message = std::string("an empty (Command::None) order does not round-trip as '0 0 0'.");
        return 0;
    }

    // ---- what is not a command is refused, and leaves the target alone --------------------
    const char* bad[] = { "", "1 2", "1 2 x", "1 2 3 zz=4", "1 2 3 u=", "1 2 3 u=4x", "1 2 3 u",
                          "1 2 3 rates=1,2,3", "1 2 3 rates=1,2,3,4,5", "a 2 3" };
    for (const char* b : bad)
    {
        CommandOrder untouched = full;
        if (CommandOrder::deserialize(b, untouched) || !sameCommand(untouched, full))
        {
            snprintf(buf, sizeof(buf), "deserialize() accepted '%s' (or changed the order it refused).", b);
            message = std::string(buf);
            return 0;
        }
    }

    // ---- ids and years are given at push time -----------------------------------------------
    year = -3500;
    while (!coordinator.empty()) coordinator.pop();

    CommandOrder rates;
    rates.command = Command::SetFundamentalRatesOrder;
    rates.parameters.factionid = 0;
    rates.parameters.rates[0] = 0.25f; rates.parameters.rates[1] = 0.25f;
    rates.parameters.rates[2] = 0.25f; rates.parameters.rates[3] = 0.25f;

    CommandOrder autoplayer;
    autoplayer.command = Command::SetAutoPlayerOrder;
    autoplayer.parameters.factionid = 0;
    autoplayer.parameters.enabled = false;

    // Read back from a log: it keeps the id and year it was issued with.
    CommandOrder replayed;
    replayed.id = 999999;
    replayed.year = -4000;
    replayed.command = Command::RegisterDependencyOrder;
    replayed.parameters.scope = DEP_SCOPE_WORLD;
    replayed.parameters.codeid = 5;

    int firstid = coordinator.nextorderid;
    coordinator.push(rates);
    coordinator.push(autoplayer);
    coordinator.push(replayed);

    std::vector<CommandOrder> queued;
    while (!coordinator.empty()) queued.push_back(coordinator.pop());
    if (queued.size() != 3 || queued[0].id != firstid || queued[1].id != firstid + 1 ||
        queued[0].year != -3500 || queued[1].year != -3500)
    {
        message = std::string("push() did not give consecutive ids and the current year.");
        return 0;
    }
    if (queued[2].id != 999999 || queued[2].year != -4000 || coordinator.nextorderid != firstid + 2)
    {
        message = std::string("push() restamped a command that already had an id (a replayed one).");
        return 0;
    }

    // ---- every processed command lands in the match log, in order -----------------------------
    if (strcmp(matchlog, "tmp/match.log") != 0)
    {
        message = std::string("setup: testcases should record into tmp/match.log.");
        return 0;
    }
    for (auto& co : queued)
        coordinator.push(co);
    processCommandOrders();

    std::ifstream in(matchlog);
    std::vector<std::string> lines;
    std::string l;
    while (std::getline(in, l))
        lines.push_back(l);
    if (lines.size() < queued.size())
    {
        snprintf(buf, sizeof(buf), "%s holds %zu lines, fewer than the %zu commands just processed.", matchlog, lines.size(), queued.size());
        message = std::string(buf);
        return 0;
    }
    size_t base = lines.size() - queued.size();
    for (size_t i = 0; i < queued.size(); i++)
    {
        CommandOrder logged;
        if (!CommandOrder::deserialize(lines[base + i], logged) || !sameCommand(logged, queued[i]))
        {
            snprintf(buf, sizeof(buf), "match log line '%s' is not processed command %zu ('%s').",
                     lines[base + i].c_str(), i, queued[i].serialize().c_str());
            message = std::string(buf);
            return 0;
        }
    }
    // And the commands really ran (the log is not a copy of the queue that bypassed them).
    if (factions[0]->rates[2] != 0.25f)
    {
        message = std::string("the logged SetFundamentalRatesOrder did not run.");
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_083::title()
{
    return std::string("CommandOrder serialization: ids and years at push, one-line round trip, malformed lines refused, every processed command in the match log.");

}

bool TestCase_083::done()
{
    return isdone;
}
bool TestCase_083::passed()
{
    return haspassed;
}
std::string TestCase_083::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_083();
}
