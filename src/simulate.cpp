#include <sys/stat.h>
#include "gamekernel.h"     // initMap/initFactions/initUnits: the REAL ones, shared with the game
#include "mapmodel.h"
#include "tiles.h"
#include "resources.h"
#include "Faction.h"
#include "City.h"
#include "units/Unit.h"
#include "units/Settler.h"
#include "units/Warrior.h"
#include "units/Worker.h"
#include "units/Horseman.h"
#include "units/Trireme.h"
#include "units/Archer.h"
#include "units/Swordman.h"
#include "units/Spearman.h"
#include "units/Axeman.h"
#include "units/Horsearcher.h"
#include "units/Galley.h"
#include "units/Scout.h"
#include "units/Warelephant.h"
#include "units/Chariot.h"
#include "units/Pretorian.h"
#include "units/Spy.h"
#include "buildings/Building.h"
#include "buildings/Palace.h"
#include "buildings/Barracks.h"
#include "buildings/Granary.h"
#include "buildings/Collosseum.h"
#include "buildings/Market.h"
#include "coordinator.h"
#include "dee.h"
#include "technologies.h"
#include "improvements.h"
#include "diplomacy.h"
#include "automation.h"
#include "engine.h"
#include "messages.h"
#include "commandline.h"
#include "usercontrols.h"

typedef std::unordered_map<int, City*> Cities;
typedef std::unordered_map<int, Unit*> Units;

extern Tiles tiles;
std::unordered_map<int,std::queue<std::string>> citynames;

Factions factions;
Units units;
Cities cities;
// Global, like the three above: engine.cpp (activateUnit) needs an extern Coordinator, and
// Unit.cpp (goTo) needs an extern Map -- both unreachable dead code in this stripped build,
// but their SYMBOLS still have to resolve at link time.
// `map`, `tiles` and `improvements` are map.cpp's -- simulate links it now, so it no
// longer declares its own copies (that was a duplicate symbol).
extern Map map;
Coordinator coordinator;
// engine.cpp:engageTrade() references `controller` to open the commerce screen -- dead code
// in this headless build (no Transport ever trades here), but the symbol must resolve.
Controller controller;
DependencyEvaluationEngine dee;
TechTree techtree;
DiplomacyTable diplomacy;
extern std::unordered_map<int, Improvement*> improvements;

std::vector<Message> messages;

extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern ImprovementBiomaRestrictions improvementbiomarestrictions;
extern MovementCost movementcosts;
extern std::unordered_map<int, int> commodityxresource;
extern std::unordered_map<int, int> prices;

int year;

bool preloadmap;

bool loadgame;
char filegame[256];

bool autoEndOfTurn = true;
bool switchVisibleFaction = false;
bool mute = false;

int mapsize;

void built() {} 




// assignProductionRates() now lives in engine.cpp, shared. The copy that used to be here was
// an older hand-written if/else chain that had DRIFTED from gamekernel.cpp's table -- it gave
// grassland FOOD 3 where the table says 1, and plain land 2 where it says 1 -- so the
// simulator and the game were quietly modelling different worlds. Both read the one table now.

// ---------------------------------------------------------------------------------------
// The graphics/sound boundary, and the whole of it.
//
// Everything above this point is the REAL game: simulate links gamekernel.cpp, engine.cpp,
// map.cpp, savegame.cpp and the rest, and runs the same world generation, the same turn and
// the same rules the windowed game does. What it does not have is a screen or speakers, so
// the handful of symbols that need one are defined here as empty bodies.
//
// Deliberately LINK-TIME stubs rather than an interface with virtual calls: the simulator
// pays nothing for the ones it never calls, and -- the point -- the real game pays nothing
// either, where these are on the per-frame path. Adding a vtable here to satisfy a headless
// build would tax every draw in the game to solve a problem the linker already solves.
//
// If one of these ever needs to DO something headless (say, a text renderer that logs), it
// stops being a stub and gets a real implementation here.

// -- openglutils.cpp: the drawing primitives everything else is built on ------------------
void drawBox(float x, float y, int sizex, int sizey, float r, float g, float b) {}
void drawString(float x, float y, float z, char* str, float scale) {}
void placeMark(float x, float y, int size, const char* modelName) {}
void placeMark(float x, float y, int sizex, int sizey, const char* modelName) {}
void placeMark(float x, float y, int sizex, int sizey, unsigned int texture) {}
void preloadCityTexture(const char* name, const char* filename, int red, int green, int blue) {}
void preloadUnitTexture(const char* name, const char* filename, int red, int green, int blue) {}

// -- cityscreenui.cpp / commerceui.cpp: whole screens -------------------------------------
void drawCityScreen(int cla, int clo, City *city) {}
void openCommerceScreen() {}
void initCoreResources() {}

// -- sounds.cpp: one anthem per faction (FactionDefinition::song) --------------------------
void vikings() {}   void romans() {}      void greeks() {}     void chinese() {}
void egyptians() {} void babylonians() {} void english() {}    void mongols() {}
void russians() {}  void zulus() {}       void germans() {}    void french() {}
void aztec() {}     void americans() {}   void indians() {}    void incan() {}
void japanese() {}  void spanish() {}

// -- globals the windowed build owns (bunmei.cpp) -----------------------------------------
// The window has no size here; nothing headless reads these, but gamekernel.cpp's faction
// setup and map.cpp's projection both reference them.
int REAL_SCREEN_WIDTH  = 0;
int REAL_SCREEN_HEIGHT = 0;

// How many civilizations to create, and which one a human would be playing. The simulator
// makes every faction an autoPlayer after initFactions() regardless (see main), so
// selectedFaction stays -1: nobody is at a keyboard.
int numCivs = 0;
int selectedFaction = -1;

// openglutils.cpp: text laid out on the tile grid.
void placeWord(float x, float y, int sizex, int sizey, const char* word, int yoffset) {}

void blocked()
{}

void war()
{}

void peace()
{}

void win()
{}

void march()
{}

void lose()
{}

// endOfYear() lives in gamekernel.cpp now, shared with the game. The copy that used to sit
// here had drifted badly -- no unit salaries, first contact commented out, no TRADE_SURPLUS
// or SCIENCE_SURPLUS perks, and no operateCityBuildings() -- so the simulator was modelling a
// materially different economy from the one it was supposed to be simulating.

void update(int value)
{
    cleanUnits();

    reSetCities();

    processGoTo();

    processWork(); 
    
    // Autoplayer
    if (factions[coordinator.a_f_id]->autoPlayer)
    {
        printf("Autoplayer for faction %d - %s\n", coordinator.a_f_id, factions[coordinator.a_f_id]->name);
        autoPlayerMoveUnits();
    }

    processCommandOrders();

    // @NOTE: Remove me if you want to wait until the user press the space bar to move ahead the end of turn.
    if (autoEndOfTurn && noMoreMovementsLeft(coordinator.a_f_id))
    {
        printf ("End of turn for faction %d - %s\n", coordinator.a_f_id, factions[coordinator.a_f_id]->name);
        coordinator.endofturn = true;
    }

    if (coordinator.endofturn)
    {
        coordinator.endofturn=false;
        factions[coordinator.a_f_id]->done();

        printf("Faction %d - %s has finished its turn.\n", coordinator.a_f_id, factions[coordinator.a_f_id]->name);

        if (coordinator.a_f_id<factions.size()-1) 
        {
            coordinator.a_f_id++;

            if (switchVisibleFaction)
                coordinator.v_f_id = coordinator.a_f_id;

            setUpFaction();  
            
            // Autoplayer
            if (factions[coordinator.a_f_id]->autoPlayer)
            {
                autoPlayerCities();
            }
        }

    }    

    if (endOfTurnForAllFactions())
    {
        printf ("All factions have finished their turn, end of year %d.\n", year);
        // Everybody played their turn, end of year, and start it over.....
        endOfYear();
        coordinator.a_f_id = 0;     // Restart the turn from the first faction.
        setUpFaction();

        // Autoplayer
        if (factions[coordinator.a_f_id]->autoPlayer)
        {
            autoPlayerCities();
        }
    }
}


int main(int argc, char *argv[]) {


    if (isPresentCommandLineParameter(argc,argv,"-seed"))
    {
        int seed = getDefaultedIntCommandLineParameter(argc,argv,"-seed",0);
        srand( seed );
        srand48(seed);
        setRandomSeed(seed);        // getRandomInteger uses its own <random> generator.
    }
    else
    {
        srand (time(NULL));
        srand48(time(NULL));
    }

    mapsize = getDefaultedIntCommandLineParameter(argc,argv,"-mapsize",DEFAULT_MAPSIZE);

    preloadmap = false;
    if (isPresentCommandLineParameter(argc,argv,"-loadmap"))
    {
        preloadmap = true;
    }

    loadgame = false;
    if (isPresentCommandLineParameter(argc,argv,"-loadgame"))
    {
        loadgame = true;
        strcpy(filegame, getCommandLineParameter(argc,argv,"-loadgame"));
        struct stat buffer;
        if (stat(filegame, &buffer) != 0) {
            std::cerr << "Error: The file " << filegame << " does not exist." << std::endl;
            exit(1);
        }
    }

    coordinator.a_f_id = 0;
    coordinator.a_u_id = 0;

    

    initTiles(tiles);
    initProductionRates(productionrates);
    initCommodities(commodityxresource);
    initPrices(prices);
    initMovementCosts(movementcosts);
    initImprovementEffort(improvementeffort);
    initImprovementResources(improvementresources);
    initImprovementBiomaRestrictions(improvementbiomarestrictions);
    initImprovements(improvements);
    initNaming(citynames);

    MapDimension dimension = getMapDimension(mapsize);
    map.init(dimension.halfheight,dimension.halfwidth);

    initMap();

    printf("Map minlat %d maxlat %d minlon %d maxlon %d\n", map.minlat, map.maxlat, map.minlon, map.maxlon);

    assignProductionRates(map);

    initFactions();

    for (Faction* faction : factions)
    {
        printf("Faction name %s Autoplayer %s\n", faction->name, faction->autoPlayer ? "true" : "false");
        if (faction)
            faction->autoPlayer = true;
    }

    initDiplomacy(diplomacy, factions.size());

    // Tech graph: one per faction, everybody starting at the root (Language). Shared with
    // gamekernel.cpp through initTechnologies() so the game and the simulator set it up the
    // same way (gamekernel.cpp is not linked into the simulator).
    initTechnologies(techtree, factions.size(), dee);

    initUnits();

    setUpFaction();

    // Whatever the starting units can see. Here, where the world has just been built -- not
    // in the per-turn path, where endOfYear() already does it.
    updateFogOfWar();

    // At this point everything is set up.


    for (auto& unit : units)
    {
        printf("Unit %d - %s at (%d,%d) for faction %d\n", unit.second->id, unit.second->name, unit.second->latitude, unit.second->longitude, unit.second->faction);
    }

    //exit(22);



    // Now the logic goes like this.
    // 1. Iterate through all the factions.
    // 2. For each faction, and activate each unit.
    // 3. For each unit, decide what to do.
    
    year = -4000;
    bool wincondition = false;
    int ticks = 0;
    // Safety valve: nothing here ever calls Faction::done() unless a unit runs out of
    // moves (switchUnitIfNoMovesLeft), so with no starting units/cities the turn never
    // ends and year is stuck forever -- this loop would otherwise spin at full CPU,
    // printing "Year -4000" indefinitely (found the hard way: filled the disk when
    // redirected to a file).
    const int MAX_TICKS = 200000;
    while (!wincondition)
    {
        if (++ticks > MAX_TICKS)
        {
            printf("Safety stop: %d ticks elapsed with year stuck at %d -- the game loop is not progressing (no units/cities to end a turn).\n", MAX_TICKS, year);
            break;
        }

        printf("Year %d\n",year);
        //update(year,map, factions,cities, units,resources,coordinator, citynames);
        update(ticks);

        if (year == 2000)
            break;
    }


    return 0;

}
