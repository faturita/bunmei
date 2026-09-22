#ifndef COMMANDORDER_H
#define COMMANDORDER_H

enum class Command {
    None=0,
    BuildCityOrder=1,
    DisbandUnitOrder=2,
    FortifyUnitOrder=3,
    SentryUnitOrder=4,
    BuildRoadOrder=5,
    BuildIrrigationOrder=6,
    BuildRailroadOrder=7,
    BuildMineOrder=8,
    // Finalize commands: pushed by processWork() once a worker's effort reaches zero,
    // to actually apply the improvement to the tile (BuildXxxOrder only puts the worker
    // into the corresponding working state).
    BuildRoad=9,
    BuildMine=10,
    BuildIrrigation=11,
    BuildRailroad=12,
    MoveUnitTo=13,
    BuildQuarryOrder=14,
    BuildCampOrder=15,
    BuildDerrickOrder=16,
    BuildPlantationOrder=17,
    BuildQuarry=18,
    BuildCamp=19,
    BuildDerrick=20,
    BuildPlantation=21,
    // Assign/deassign a city tile for work (the city UI's tile click): a toggle, same as
    // City::assignWorkingTile(coordinate) -- processCommandOrders() decides assign vs.
    // deassign from the tile's current state, same as the function it replaces at the call
    // site. parameters.cityid identifies the city; parameters.latitude/longitude carry the
    // tile's offset RELATIVE to the city (-3..3), unlike every other command's absolute map
    // coordinates.
    AssignWorkTileOrder=22,
    // Repopulates a city's buildable list (the city UI's "Change" list): clears
    // City::buildable and rebuilds it via populateCityBuildables(). A city's buildable list
    // starts empty when the city is founded (BuildCityOrder) and is only filled the first
    // time the player opens the Change screen for it. parameters.cityid identifies the city.
    PopulateBuildableOrder=23,
    // Loads up to 100 units of one commodity/mfggood (parameters.resourceid) from a city
    // (parameters.cityid) onto a Transport (parameters.spawnid, the active unit -- must be
    // docked/stationed at that city). Stacks onto an already-boarded Shippable with the same
    // resource id (up to the 100 cap) instead of taking a second cargo slot; only creates a
    // new Commodity/MfgGood, and takes a slot, if none of that type is aboard yet.
    LoadCargoOrder=24,
    // Reverse of LoadCargoOrder: moves one boarded Shippable resource (parameters.resourceid,
    // aboard the Transport in parameters.spawnid) back into the city's stockpile
    // (parameters.cityid) and removes/deletes it from the Transport.
    UnloadCargoOrder=25,
    // Commerce screen "buy" arrow: like LoadCargoOrder (moves up to 100 of parameters.resourceid
    // from the city stockpile onto the Transport in parameters.spawnid), but also pays for it --
    // quantity is additionally capped by what the buying faction can afford at prices[resourceid],
    // faction->coins goes down and city->resources[COINS] goes up by quantity*price.
    BuyResourceOrder=26,
    // Commerce screen "sell" arrow: like UnloadCargoOrder (moves the boarded resource stack
    // back into the city stockpile), but also pays out -- city->resources[COINS] goes down
    // and faction->coins goes up by quantity*prices[resourceid] (quantity capped by what the
    // city can afford).
    SellResourceOrder=27,
    // Sets the calling faction's four "fundamental rates" -- the share of a city's TRADE that
    // endOfYear() converts into COINS, SCIENCE, CULTURE and LUXURY respectively (Faction::rates).
    // parameters.factionid says whose, parameters.rates carries the four values. Pushed by the
    // /fundamental teletype command.
    SetFundamentalRatesOrder=28,
    // Releases ONE surplus working tile from a city (parameters.cityid), via
    // City::deAssignWorkingTile() -- which drops the first worked tile it finds while the
    // city is working more than pop+1 of them, and does nothing when it is not. That is the
    // whole contract: it takes no tile, because it is not "stop working tile X" (that is
    // AssignWorkTileOrder, whose toggle already deassigns a NAMED tile) but "shed the one
    // tile this city is no longer entitled to", which is what a shrinking population needs.
    //
    // Exists so nothing has to reach into the City to do it: the AI and a remote player both
    // drive the game exclusively through commands, so every state change a turn can make
    // needs to be expressible as one. endOfYear() (bunmei.cpp/simulate.cpp) and
    // engine.cpp's own population path still call the method directly -- they run outside
    // the command queue, and deferring them by a frame would change when a tile is freed --
    // so this is the entry point for everything else.
    DeAssignWorkTileOrder=29,
    // Work ONE named tile / stop working ONE named tile. parameters.cityid is the city;
    // parameters.latitude/longitude are the tile's offset RELATIVE to the city (-3..3), same
    // convention as AssignWorkTileOrder and unlike every other command's absolute coordinates.
    //
    // These are explicit where AssignWorkTileOrder is a TOGGLE, and that is the whole reason
    // they exist: a toggle requires the sender to already know the tile's current state, which
    // a UI click does and neither the AI nor a remote player reliably does -- two toggles
    // racing on the same tile cancel out, and a stale view flips the wrong way. Each of these
    // states an intended outcome and does NOTHING if it already holds:
    //   AssignTileOrder   -- no-op if the tile is already worked, if the city has no
    //                        assignment left (numberOfWorkingTiles() == pop+1), if the tile is
    //                        out of range or the centre, or if another faction/city holds it.
    //   DeAssignTileOrder -- no-op if the city is not working that tile, or it is out of range
    //                        or the centre. Never limited by the allowance.
    // So they are idempotent: sending either one twice has the same effect as sending it once.
    AssignTileOrder=30,
    DeAssignTileOrder=31,
    // Makes parameters.spawnid the active/selectable unit and wakes it out of whatever
    // passive state it was in: a fortified unit packs up, a sentried one wakes, and a
    // working one has its improvement INTERRUPTED (Unit::completed(), no finalize -- a later
    // order starts that effort over). engine.cpp:activateUnit() is the shared body.
    //
    // parameters.factionid is the faction claiming the unit, and the handler checks the unit
    // actually belongs to it: selecting a unit is the one action a remote client could
    // otherwise use to reach into somebody else's army, and the caller's own check is only a
    // local convenience (it cannot be trusted once the caller is across a network).
    ActivateUnitOrder=32,
    // Turns AI control of a faction on or off (Faction::autoPlayer). parameters.factionid is
    // whose, parameters.enabled is the new value. Pushed by the /autoplayer teletype command.
    SetAutoPlayerOrder=33,
    // Sets the relation between TWO factions (parameters.factionid and
    // parameters.targetfactionid) to parameters.status, a DiplomaticStatus (diplomacy.h).
    // The table is undirected, so one entry covers both directions.
    //
    // This is the one command that changes state belonging to somebody else as much as to the
    // sender, which is why the handler re-validates everything the caller checked: both
    // factions must exist, they must not be the same faction, and the status must be a real
    // one. Pushed by the peace/war dialog (usercontrols.cpp).
    SetDiplomacyOrder=34,
    // Registers a codes.h dependency code (parameters.codeid) in the Dependency Evaluation
    // Engine at parameters.scope -- DEP_SCOPE_WORLD, DEP_SCOPE_FACTION (parameters.factionid)
    // or DEP_SCOPE_CITY (parameters.cityid). Pushed by the /enable teletype cheat, which is
    // the only thing that grants capabilities out of band; routing it through the queue makes
    // it replayable and auditable like every other state change.
    RegisterDependencyOrder=35,
    // Sets what a city builds next: clears its productionQueue and queues
    // parameters.selectedbuildableid, in the city parameters.cityid.
    //
    // By BuildableId (buildable.h), which is what the city screen, an AI or a remote player
    // each resolve their own selection to. Not by the row index the Change list was clicked
    // at -- that is a property of a rendered list (its scroll offset, and whatever the
    // buildable set happened to be when it was drawn), so two sides that disagree about that
    // list would silently queue the wrong thing. Not by name either: a name is display text
    // that can change for presentation reasons, while an id is the thing's identity.
    //
    // The handler still refuses an id the city cannot currently build, which is what makes
    // accepting one from a caller safe.
    ChangeProductionOrder=36,
    // Sends a unit (parameters.spawnid) to a destination (parameters.latitude/longitude, REAL
    // map coordinates): sets the automated-movement target the AI pathfinder then walks, i.e.
    // Unit::goTo(). Not MoveUnitTo, which is a single STEP onto an adjacent tile -- this is
    // "head there over however many turns it takes".
    //
    // The handler checks the unit belongs to parameters.factionid, the same as
    // ActivateUnitOrder: a destination is an order to somebody's army.
    SetUnitDestinationOrder=37,
    // Sets which technology a faction is researching (parameters.techid, for
    // parameters.factionid). Pushed by the research selector dialog, and available to an
    // AI or a remote player, which is the point -- TechTree::setResearchTarget() already
    // refuses a technology outside that faction's Frontier, but nothing could reach it except
    // the dialog's own callback.
    SetResearchTargetOrder=38
};

// parameters.scope for RegisterDependencyOrder. Deliberately NOT the dee.h context ids: those
// are an encoding (worldContext()/factionContext(id)/cityContext(id)) that the handler builds,
// so a command never carries a pre-encoded context it could get wrong.
#define DEP_SCOPE_WORLD   0
#define DEP_SCOPE_FACTION 1
#define DEP_SCOPE_CITY    2

struct commandparameters
{
    // Unit id -- for any command that addresses a unit (MoveUnitTo, BuildCityOrder,
    // DisbandUnitOrder, FortifyUnitOrder, SentryUnitOrder, BuildRoadOrder/BuildIrrigationOrder/
    // BuildMineOrder/BuildRailroadOrder/BuildQuarryOrder/BuildCampOrder/BuildDerrickOrder/
    // BuildPlantationOrder). Set by the caller at push() time from whichever unit is driving
    // the order (coordinator.a_u_id for the human player, the AI's own unit pointer, etc.) so
    // processCommandOrders() reads the unit id from here instead of coordinator.a_u_id --
    // the command carries everything it needs to run on its own.
    int spawnid;

    // Faction id -- same reasoning as spawnid above, for commands that need to know which
    // faction issued them (BuildCityOrder's capital/city-naming logic, and the
    // nextMovableUnitId() call every unit-order handler makes once it's done). Set from the
    // acting unit's own ->faction field (or coordinator.a_f_id where no unit is involved yet),
    // NOT read from coordinator.a_f_id at process time.
    int factionid;

    // City id -- for any command that addresses a city (e.g. AssignWorkTileOrder). Kept
    // separate from spawnid (units), so reusing one for the other doesn't risk colliding if
    // a future command ever needs both.
    int cityid;

    int latitude;
    int longitude;

    // The BuildableId (buildable.h) a ChangeProductionOrder wants queued, looked up in the
    // city's own buildable list. (Replaces a dead `char buf[20]` that nothing read.)
    int selectedbuildableid;

    // Commodity/MfgGood id (resources.h) -- for LoadCargoOrder/UnloadCargoOrder. Kept
    // separate from latitude/longitude, which those two commands don't use.
    int resourceid;

    // The four TRADE conversion shares (COINS, SCIENCE, CULTURE, LUXURY) -- for
    // SetFundamentalRatesOrder only. Same order and meaning as Faction::rates.
    float rates[4];

    // The OTHER faction -- for a command addressing a PAIR of them (SetDiplomacyOrder), where
    // factionid is the one issuing it.
    int targetfactionid;

    // A DiplomaticStatus (diplomacy.h) -- SetDiplomacyOrder.
    int status;

    // A codes.h TECH_* technology -- SetResearchTargetOrder.
    int techid;

    // On/off -- SetAutoPlayerOrder.
    bool enabled;

    // DEP_SCOPE_* above, and the codes.h code to register -- RegisterDependencyOrder.
    int scope;
    int codeid;
};

struct CommandOrder
{
    Command command;
    commandparameters parameters;
};

struct controlregister
{
    // R+,F-
    float thrust=0;

    // ModAngleX
    float roll=0;

    // ModAngleY
    float pitch=0;

    // ModAngleZ
    float yaw=0;

    // ModAngleP
    float precesion=0;

    float bank=0;
};

struct ControlStructure {
    int controllingid;
    struct controlregister registers;
    int faction;
    unsigned long sourcetimer;
    CommandOrder order;
};


#endif // COMMANDORDER_H
