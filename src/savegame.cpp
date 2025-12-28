
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
#include "units/Archer.h"
#include "units/Horsearcher.h"
#include "units/Horseman.h"
#include "units/Chariot.h"
#include "units/Trireme.h"
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

#include "savegame.h"

extern std::vector<Faction*> factions;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;

extern std::unordered_map<int,std::queue<std::string>> citynames;

extern Map map;

extern int year;

void savegame(const char* filename)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        printf("Error opening %s for writing.\n", filename);
        return;
    }

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

        // The buildings and resources are missing.

        // Save resources
        size_t resource_count = c->resources.size();
        out.write(reinterpret_cast<const char*>(&resource_count), sizeof(resource_count));
        for (const auto& res : c->resources) {
            out.write(reinterpret_cast<const char*>(&res), sizeof(res));
        }

        // Save working tiles
        int number_of_working_tiles = c->numberOfWorkingTiles();
        out.write(reinterpret_cast<const char*>(&number_of_working_tiles), sizeof(number_of_working_tiles));
        coordinate co = map.to_screen(c->latitude,c->longitude);
        for(int lats=-3;lats<=3;lats++)
            for(int lons=-3;lons<=3;lons++)
            {
                int la= co.lat + lats;
                int lo = co.lon + lons;

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

    out.close();
    printf("Game saved to %s\n", filename);
}


void loadCities(std::ifstream& in)
{
    // Load cities
    size_t city_count = 0;
    in.read(reinterpret_cast<char*>(&city_count), sizeof(city_count));
    printf("Loading %zu cities...\n", city_count);
    for (size_t i = 0; i < city_count; ++i) {
        City* c = new City(&map, 0, 0, 0, 0); // Temporary values
        in.read(reinterpret_cast<char*>(&c->id), sizeof(c->id));
        bool isCapital;
        in.read(reinterpret_cast<char*>(&isCapital), sizeof(isCapital));
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
        c->resources.resize(resource_count);
        for (size_t j = 0; j < resource_count; ++j) {
            in.read(reinterpret_cast<char*>(&c->resources[j]), sizeof(c->resources[j]));
        }

        // Load working tiles
        int number_of_working_tiles = 0;
        in.read(reinterpret_cast<char*>(&number_of_working_tiles), sizeof(number_of_working_tiles));
        for (int j = 0; j < number_of_working_tiles; ++j) {
            int lats = 0;
            int lons = 0;
            in.read(reinterpret_cast<char*>(&lats), sizeof(lats));
            in.read(reinterpret_cast<char*>(&lons), sizeof(lons));
            c->assignWorkingTile(coordinate(lats, lons));
            printf("Loaded working tile at offset (%d,%d) for city %s\n", lats, lons, c->name);
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


void loadUnits(std::ifstream& in)
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
            u = new Trireme(); // Replace with Trireme class if available
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

        printf("Loaded unit: %s (ID: %d, Faction: %d, Location: (%d, %d), Available Moves: %d)\n",
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

