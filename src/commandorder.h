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
    // faction->coins goes down and city->coreresources[COINS] goes up by quantity*price.
    BuyResourceOrder=26,
    // Commerce screen "sell" arrow: like UnloadCargoOrder (moves the boarded resource stack
    // back into the city stockpile), but also pays out -- city->coreresources[COINS] goes down
    // and faction->coins goes up by quantity*prices[resourceid] (quantity capped by what the
    // city can afford).
    SellResourceOrder=27
};

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
    char buf[20];

    // Commodity/MfgGood id (resources.h) -- for LoadCargoOrder/UnloadCargoOrder. Kept
    // separate from latitude/longitude, which those two commands don't use.
    int resourceid;
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
