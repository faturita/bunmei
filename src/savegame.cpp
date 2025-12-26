
#include <fstream>

#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"
#include "coordinator.h"

#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"

#include "messages.h"
#include "engine.h"

#include "tiles.h"

#include "savegame.h"

extern std::vector<Faction*> factions;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;

void savegame(const char* filename)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        printf("Error opening %s for writing.\n", filename);
        return;
    }

    // Save factions
    size_t faction_count = factions.size();
    out.write(reinterpret_cast<const char*>(&faction_count), sizeof(faction_count));
    for (auto& f : factions) {
        out.write(reinterpret_cast<const char*>(&f->id), sizeof(f->id));
        out.write(reinterpret_cast<const char*>(f->name), sizeof(f->name));
        out.write(reinterpret_cast<const char*>(&f->red), sizeof(f->red));
        out.write(reinterpret_cast<const char*>(&f->green), sizeof(f->green));
        out.write(reinterpret_cast<const char*>(&f->blue), sizeof(f->blue));
        out.write(reinterpret_cast<const char*>(f->rates), sizeof(f->rates));
        out.write(reinterpret_cast<const char*>(&f->autoPlayer), sizeof(f->autoPlayer));
    }

    // Save cities
    size_t city_count = cities.size();
    out.write(reinterpret_cast<const char*>(&city_count), sizeof(city_count));
    for (auto& pair : cities) {
        City* c = pair.second;
        out.write(reinterpret_cast<const char*>(&c->id), sizeof(c->id));
        out.write(reinterpret_cast<const char*>(&c->faction), sizeof(c->faction));
        out.write(reinterpret_cast<const char*>(&c->latitude), sizeof(c->latitude));
        out.write(reinterpret_cast<const char*>(&c->longitude), sizeof(c->longitude));
        out.write(reinterpret_cast<const char*>(&c->pop), sizeof(c->pop));
        out.write(reinterpret_cast<const char*>(&c->shields), sizeof(c->shields));
        out.write(reinterpret_cast<const char*>(&c->food), sizeof(c->food));
        out.write(reinterpret_cast<const char*>(&c->shields), sizeof(c->shields));

        // Save city name
        size_t name_len = strlen(c->name);
        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(c->name, name_len);

        // The buildings and resources are missing.
    }

    // Save units
    size_t unit_count = units.size();
    out.write(reinterpret_cast<const char*>(&unit_count), sizeof(unit_count));
    for (auto& pair : units) {
        Unit* u = pair.second;
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

    out.close();
    printf("Game saved to %s\n", filename);
}
/** 
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

    // Load cities
    size_t city_count = 0;
    in.read(reinterpret_cast<char*>(&city_count), sizeof(city_count));
    for (size_t i = 0; i < city_count; ++i) {
        City* c = new City(&map, 0, 0, 0, 0); // Temporary values
        in.read(reinterpret_cast<char*>(&c->id), sizeof(c->id));
        in.read(reinterpret_cast<char*>(&c->faction), sizeof(c->faction));
        in.read(reinterpret_cast<char*>(&c->latitude), sizeof(c->latitude));
        in.read(reinterpret_cast<char*>(&c->longitude), sizeof(c->longitude));
        in.read(reinterpret_cast<char*>(&c->pop), sizeof(c->pop));
        in.read(reinterpret_cast<char*>(&c->shields), sizeof(c->shields));
        in.read(reinterpret_cast<char*>(&c->food), sizeof(c->food));

        // Load city name
        size_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        in.read(c->name, name_len);
        c->name[name_len] = '\0'; // Null-terminate the string

        cities[c->id] = c;
    }

    // Load units
    size_t unit_count = 0;
    in.read(reinterpret_cast<char*>(&unit_count), sizeof(unit_count));

    for (size_t i = 0; i < unit_count; ++i) {
        Unit* u = new Unit(0, 0, 0, 0); // Temporary values
        in.read(reinterpret_cast<char*>(&u->id), sizeof(u->id));
        in.read(reinterpret_cast<char*>(&u->faction), sizeof(u->faction));
        in.read(reinterpret_cast<char*>(&u->latitude), sizeof(u->latitude));
        in.read(reinterpret_cast<char*>(&u->longitude), sizeof(u->longitude));
        in.read(reinterpret_cast<char*>(&u->availablemoves), sizeof(u->availablemoves));

        // Load unit name
        size_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        in.read(u->name, name_len);
        u->name[name_len] = '\0'; // Null-terminate the string

        units[u->id] = u;
    }

}**/

