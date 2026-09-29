#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "commandorder.h"

// CommandOrder on the wire (the match log, and later the network): one text line,
//
//     <id> <year> <command> key=value key=value ...
//
// e.g.  "17 -3999 1 u=42"  -- order 17, year -3999, BuildCityOrder (1) for unit 42, faction 0.
//
// The command is its Command number and each parameter a short fixed key; only parameters that
// are not zero are written, and a missing one decodes as zero (CommandOrder's default). A
// typical command sets two to four of them, so a line is a few bytes, and it stays greppable.
//
// The keys and the Command numbers are the format: never rename or renumber them.

struct IntField
{
    const char* key;
    int commandparameters::* member;
};

static const IntField INT_FIELDS[] = {
    { "u",   &commandparameters::spawnid },
    { "f",   &commandparameters::factionid },
    { "c",   &commandparameters::cityid },
    { "lat", &commandparameters::latitude },
    { "lon", &commandparameters::longitude },
    { "b",   &commandparameters::selectedbuildableid },
    { "r",   &commandparameters::resourceid },
    { "tf",  &commandparameters::targetfactionid },
    { "s",   &commandparameters::status },
    { "t",   &commandparameters::techid },
    { "sc",  &commandparameters::scope },
    { "cd",  &commandparameters::codeid },
};
// Plus "e" (enabled, written as 1) and "rates" (the four floats, comma separated).

std::string CommandOrder::serialize() const
{
    std::ostringstream out;
    out << id << ' ' << year << ' ' << (int)command;

    for (const IntField& f : INT_FIELDS)
        if (parameters.*f.member != 0)
            out << ' ' << f.key << '=' << parameters.*f.member;

    if (parameters.enabled)
        out << " e=1";

    bool anyrate = false;
    for (int i = 0; i < FUNDAMENTAL_RATES; i++)
        if (parameters.rates[i] != 0.0f)
            anyrate = true;
    if (anyrate)
    {
        // %.9g round-trips a float exactly.
        out << " rates=";
        char buf[32];
        for (int i = 0; i < FUNDAMENTAL_RATES; i++)
        {
            snprintf(buf, sizeof(buf), "%s%.9g", i ? "," : "", parameters.rates[i]);
            out << buf;
        }
    }

    return out.str();
}

// The whole of `text` must be an integer.
static bool parseInt(const std::string& text, int& value)
{
    if (text.empty())
        return false;
    char* end = nullptr;
    long v = strtol(text.c_str(), &end, 10);
    if (*end != '\0')
        return false;
    value = (int)v;
    return true;
}

bool CommandOrder::deserialize(const std::string& line, CommandOrder& co)
{
    std::istringstream in(line);
    std::string idtext, yeartext, commandtext;
    CommandOrder r;
    int command = 0;

    if (!(in >> idtext >> yeartext >> commandtext) ||
        !parseInt(idtext, r.id) || !parseInt(yeartext, r.year) || !parseInt(commandtext, command))
        return false;
    r.command = (Command)command;

    std::string token;
    while (in >> token)
    {
        size_t eq = token.find('=');
        if (eq == std::string::npos)
            return false;
        std::string key = token.substr(0, eq);
        std::string value = token.substr(eq + 1);

        bool known = false;
        for (const IntField& f : INT_FIELDS)
            if (key == f.key)
            {
                if (!parseInt(value, r.parameters.*f.member))
                    return false;
                known = true;
            }
        if (known)
            continue;

        if (key == "e")
        {
            int enabled = 0;
            if (!parseInt(value, enabled))
                return false;
            r.parameters.enabled = (enabled != 0);
        }
        else if (key == "rates")
        {
            float v[FUNDAMENTAL_RATES];
            int consumed = 0;
            if (sscanf(value.c_str(), "%f,%f,%f,%f%n", &v[0], &v[1], &v[2], &v[3], &consumed) != FUNDAMENTAL_RATES ||
                consumed != (int)value.size())
                return false;
            for (int i = 0; i < FUNDAMENTAL_RATES; i++)
                r.parameters.rates[i] = v[i];
        }
        else
            return false;
    }

    co = r;
    return true;
}
