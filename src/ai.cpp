#include "ai.h"

#include "boost/mpl/and.hpp"

namespace mpl=boost::mpl;

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/topological_sort.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/properties.hpp>
#include <boost/range/algorithm.hpp>

#include <boost/property_map/property_map.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/property_map/transform_value_property_map.hpp>

#include <boost/phoenix.hpp>

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <deque>
#include <iterator>
#include <set>
#include <queue>


using boost::phoenix::arg_names::arg1;

#include <iostream>

#include "map.h"
#include "units/Unit.h"
#include "City.h"
#include "Faction.h"
#include "resources.h"
#include "map.h"
#include "units/Settler.h"
#include "engine.h"
#include "tiles.h"

extern bool war;

struct CoordinateVertex {
    int lat;
    int lon;

    int pred;

    int dist;

};

struct Edge {
    double cost = 0;

    double getWeight(int i) const
    {
        return i*cost;
    }
};

using Tree = 
    boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, CoordinateVertex, Edge>; // vectors for vertex and edges, bidirectional graph, vertex property and edge property


extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;


template<typename Condition>
Tree buildGenericTraversalTree(Condition condition)
{
    Tree tree;

    // (lat,lon) -> vertex descriptor index: the per-tile find_if lookups made this
    // function O(V^2) and dominated the cost of every goTo call on big maps.
    std::unordered_map<int, Tree::vertex_descriptor> index;
    int width = map.maxlon - map.minlon;
    auto key = [width](int lat, int lon) { return (lat - map.minlat)*width + (lon - map.minlon); };

    // Add all the land vertices into the graph.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for(int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (condition(lat, lon))
            {
                auto v = add_vertex({lat,lon,0,0}, tree);
                index[key(lat,lon)] = v;
            }
        }

    // Now, map all the connections between land mapcells.  This allow a unit to move from one land cell to another.
    for(int lat=map.minlat;lat<map.maxlat;lat++ )
        for(int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (condition(lat, lon))
            {
                auto start = index[key(lat,lon)];

                for(int i=-1;i<=1;i++)
                    for(int j=-1;j<=1;j++)
                    {
                        if (i==0 && j==0)
                            continue;

                        coordinate s = map.adjust(lat,lon,i,j);

                        int latt = s.lat;
                        int lonn = s.lon;

                        if (condition(latt, lonn))
                        {
                            auto end = index.find(key(latt,lonn));
                            // @FIXME: Add terrain cost here on the edge.

                            if (end != index.end())
                            {
                                add_edge(start, end->second, Edge{1}, tree);
                            }
                        }
                    }
            }
        }

    return tree;
}

int determineLandMass(coordinate c)
{

    Tree tree = buildGenericTraversalTree([](int lat, int lon) {
        return map.peek(lat,lon).code == LAND;
    });

    // Ok, now with the Tree I can start on the coordinate c and count how many nodes are reachable.
    auto vv = boost::make_iterator_range(vertices(tree));
    int lat = c.lat;
    int lon = c.lon;
    auto source = find_if(vv, [&, lat,lon](auto vd) {
        return tree[vd].lat == lat && tree[vd].lon == lon;
    });

    if (source == vv.end())
        return 0;

    dijkstra_shortest_paths(tree, *source, predecessor_map( get(&CoordinateVertex::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&CoordinateVertex::dist, tree)));

    int reachablecount = 0;
    for(auto iter=vv.begin(); iter!=vv.end(); iter++)
    {
        auto vd = *iter;
        if (tree[vd].dist < std::numeric_limits<int>::max())
            reachablecount++;
    }
    return reachablecount;

}

Tree buildTraversalTree(int faction)
{
    Tree tree;

    // Add all the land vertices into the graph.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for(int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (map.peek(lat,lon).code == LAND && 
                (
                map.peek(lat,lon).isUnassignedLand() || 
                map.peek(lat,lon).getOwnedBy() == faction || 
                (map.peek(lat,lon).getOwnedBy() != faction && war)
                ) )
                auto v = add_vertex({lat,lon,0,0}, tree);
        }

    auto vv = boost::make_iterator_range(vertices(tree));

    // Now, map all the connections between land mapcells.  This allow a unit to move from one land cell to another.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for(int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (map.peek(lat,lon).code == LAND && 
                (
                map.peek(lat,lon).isUnassignedLand() || 
                map.peek(lat,lon).getOwnedBy() == faction || 
                (map.peek(lat,lon).getOwnedBy() != faction && war)
                ) )
            {
                auto start = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });
                
                for(int i=-1;i<=1;i++)
                    for(int j=-1;j<=1;j++)
                    {
                        if (i==0 && j==0)
                            continue;

                        coordinate s = map.adjust(lat,lon,i,j);

                        int latt = s.lat;
                        int lonn = s.lon;

                        if (map.peek(latt,lonn).code == LAND && 
                        (
                        map.peek(latt,lonn).isUnassignedLand() || 
                        map.peek(latt,lonn).getOwnedBy() == faction || 
                        (map.peek(latt,lonn).getOwnedBy() != faction && war)
                        ) )
                        {
                            auto end = find_if(vv, [&, latt,lonn,i,j](auto vd) { return tree[vd].lat == latt && tree[vd].lon == lonn; });
                            // @FIXME: Add terrain cost here on the edge.
                            if (end != vv.end())
                            {
                                add_edge(*start, *end, Edge{1}, tree);
                            }
                        }
                    }
            }
        }

    return tree;
}



// Build the tree and the path to the destination.
// Calculate the next movement based on Dijkstra algorithm.
coordinate goTo(Unit* unit, bool &ok, int targetlat, int targetlon)
{
    MOVEMENT_TYPE movementType = unit->getMovementType();
    
    // Determine what terrain this unit can traverse
    int validTerrain = (movementType == OCEANTYPE) ? OCEAN : LAND;
    
    // Check if unit is on valid terrain for its type
    if (map.peek(unit->latitude,unit->longitude).code != validTerrain)
    {
        ok = false;
        return coordinate(0,0);
    }
    
    // Do NOT refuse pathfinding because of who owns the tile the unit is STANDING on:
    // enemy cities claim working tiles every year, so land can flip owner under a unit
    // (or a unit can be left inside conquered territory), and refusing to path out froze
    // the game: autoPlayer re-issued the same GoTo every tick and the turn never ended.
    // Each step of the path is still validated by moveUnit/attack when it is executed.

    // Normalize the target into real map coordinates (the spheroid wraps), so the vertex lookup can find it.
    coordinate normalized = map.adjust(targetlat,targetlon,0,0);
    targetlat = normalized.lat;
    targetlon = normalized.lon;

    // Naval units can GoTo a LAND target (a coast to disembark on, a city to dock in):
    // include the target tile in the ocean tree so the path ends with one step onto it.
    // moveUnit resolves that final step (land()/dockInCity()).
    bool navalToLand = (movementType == OCEANTYPE) && map.peek(targetlat,targetlon).code == LAND;

    // Build the traversal tree based on unit's movement type
    Tree tree = buildGenericTraversalTree([validTerrain, navalToLand, targetlat, targetlon](int lat, int lon) {
        if (navalToLand && lat==targetlat && lon==targetlon)
            return true;
        return map.peek(lat,lon).code == validTerrain;
    });


    auto vv = boost::make_iterator_range(vertices(tree));

    // At this point, the tree is built.

    int lat = unit->latitude;
    int lon = unit->longitude;
    auto source = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    if (source == vv.end())
    {
        ok = false;
        printf("Start (%d,%d) is not in the traversal tree...\n", lat, lon);
        return coordinate(0,0);
    }

    printf("Start %03d: %02d %02d - ", *source, lat, lon);

    dijkstra_shortest_paths(tree, *source, predecessor_map( get(&CoordinateVertex::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&CoordinateVertex::dist, tree)));

    // Track back the result until you find where the unit is located now.
    lat = targetlat;
    lon = targetlon;
    auto target = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    if (target == vv.end())
    {
        ok = false;
        printf("Target (%d,%d) is not in the traversal tree...\n", lat, lon);
        return coordinate(0,0);
    }

    printf("Target %03d: %02d,%d\n",*target, lat,lon);

    int lasttarget=-1;
    while(tree[*target].pred != *target)
    {
        printf("Target %03d: %02d,%d Predecessor %03d\n",*target, lat,lon,tree[*target].pred);
        int pred = tree[*target].pred;

        // Check if prd is a valid element in the graph.
        if (pred < 0 || pred >= (int)num_vertices(tree))
        {
            ok = false;
            printf("There is no way to get there...\n");
            return coordinate(0,0);
        }

        lat = tree[pred].lat;
        lon = tree[pred].lon;

        lasttarget = *target;

        target = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });
    }

    auto vpair = vertices(tree);
    for(auto iter=vpair.first; iter!=vpair.second; iter++)
    {
        auto vd = *iter;
        //std::cout << "Vertex " << vd << " has lat " << tree[vd].lat << " and lon " << tree[vd].lon << " Predecessor: " << tree[vd].pred << std::endl;
    }

    if (lasttarget == -1)
    {
        ok = false;
        printf("No way to get there...\n");
        return coordinate(0,0);
    }

    ok = true;
    printf("Next movement: %d %d %d\n",lasttarget,tree[lasttarget].lat, tree[lasttarget].lon);
    return coordinate(tree[lasttarget].lat, tree[lasttarget].lon);

}

coordinate goTo(Unit* unit, bool &ok)
{
    return goTo(unit, ok, unit->target.lat, unit->target.lon);
}

// Send the unit towards the CLOSEST reachable candidate with a single Dijkstra run.
// Calling goTo once per candidate (one full Dijkstra each) froze the game when many
// units were alive.  Returns false when no candidate is reachable.
bool goToNearest(Unit* unit, const std::vector<coordinate> &candidates)
{
    if (candidates.empty())
        return false;

    MOVEMENT_TYPE movementType = unit->getMovementType();
    int validTerrain = (movementType == OCEANTYPE) ? OCEAN : LAND;

    if (map.peek(unit->latitude,unit->longitude).code != validTerrain)
        return false;

    Tree tree = buildGenericTraversalTree([validTerrain](int lat, int lon) {
        return map.peek(lat,lon).code == validTerrain;
    });

    auto vv = boost::make_iterator_range(vertices(tree));

    int lat = unit->latitude;
    int lon = unit->longitude;
    auto source = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    if (source == vv.end())
        return false;

    dijkstra_shortest_paths(tree, *source, predecessor_map( get(&CoordinateVertex::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&CoordinateVertex::dist, tree)));

    // Every vertex now holds its distance from the unit: pick the closest candidate.
    int bestdist = std::numeric_limits<int>::max();
    coordinate best(0,0);
    for(auto &c:candidates)
    {
        coordinate n = map.adjust(c.lat,c.lon,0,0);
        auto t = find_if(vv, [&, n](auto vd) { return tree[vd].lat == n.lat && tree[vd].lon == n.lon; });
        // dist 0 is the unit's own tile: goTo cannot path to itself and the AI
        // would re-issue the same target forever, stalling the turn.
        if (t != vv.end() && tree[*t].dist > 0 && tree[*t].dist < bestdist)
        {
            bestdist = tree[*t].dist;
            best = n;
        }
    }

    if (bestdist == std::numeric_limits<int>::max())
        return false;

    unit->goTo(best.lat, best.lon);
    return true;
}

#include "Faction.h"
#include "usercontrols.h"
#include "coordinator.h"
#include "units/Warrior.h"
#include "units/Settler.h"
#include "units/Horseman.h"
#include "units/Archer.h"
#include "units/Swordman.h"


extern Coordinator coordinator;
extern Controller controller;

#include <cmath>

bool isNearbyOrSameContinent(Unit* a, Unit* b, float maxDistance = 3.0f) {
    float dx = static_cast<float>(a->latitude - b->latitude);
    float dz = static_cast<float>(a->longitude - b->longitude);
    float dist = std::sqrt(dx * dx + dz * dz);
    // TODO: Replace with continent check if available
    return dist <= maxDistance;
}


int getNumberOfCities(int f_id)
{
    int count = 0;
    for(auto& [cid,c]:cities)
    {
        if (c->faction == f_id)
            count++;
    }
    return count;
}

#define CITY_SPACING 3          // Chebyshev distance to any other city must exceed this (cities work a 7x7 area).
#define MIN_FREE_LAND 6         // Minimum unassigned land tiles around a spot for the new city to grow.

// Check if any city is closer than (or at) mindist, considering the longitude wrap.
bool cityCloserThan(int lat, int lon, int mindist)
{
    int width = map.maxlon - map.minlon;
    for(auto& [cid,c]:cities)
    {
        int dlat = abs(c->latitude - lat);
        int dlon = abs(c->longitude - lon);
        if (dlon > width/2) dlon = width - dlon;
        if (std::max(dlat,dlon) <= mindist)
            return true;
    }
    return false;
}

// A good spot for a new city: free land, far enough from other cities, and with enough free land around to grow.
bool isGoodCitySpot(int lat, int lon)
{
    coordinate n = map.adjust(lat,lon,0,0);

    if (map.peek(n.lat,n.lon).code != LAND || !map.peek(n.lat,n.lon).isUnassignedLand())
        return false;

    if (cityCloserThan(n.lat,n.lon,CITY_SPACING))
        return false;

    int freeland = 0;
    for(int i=-3;i<=3;i++)
        for(int j=-3;j<=3;j++)
        {
            coordinate s = map.adjust(n.lat,n.lon,i,j);
            if (map.peek(s.lat,s.lon).code == LAND && map.peek(s.lat,s.lon).isUnassignedLand())
                freeland++;
        }

    return freeland >= MIN_FREE_LAND;
}

// Find the closest good spot for a new city on the same landmass (BFS over land from the starting coordinate).
coordinate findCitySpot(coordinate from, bool &found)
{
    std::set<std::pair<int,int>> visited;
    std::queue<coordinate> q;

    coordinate start = map.adjust(from.lat,from.lon,0,0);
    q.push(start);
    visited.insert({start.lat,start.lon});

    while(!q.empty())
    {
        coordinate c = q.front(); q.pop();

        if (isGoodCitySpot(c.lat,c.lon))
        {
            found = true;
            return c;
        }

        for(int i=-1;i<=1;i++)
            for(int j=-1;j<=1;j++)
            {
                if (i==0 && j==0)
                    continue;

                coordinate s = map.adjust(c.lat,c.lon,i,j);

                if (map.peek(s.lat,s.lon).code == LAND && visited.count({s.lat,s.lon})==0)
                {
                    visited.insert({s.lat,s.lon});
                    q.push(s);
                }
            }
    }

    found = false;
    return from;
}

void autoPlayerMoveUnits()
{
    if (units.find(coordinator.a_u_id)!=units.end())
    {
        Unit *unit = units[coordinator.a_u_id];
        if(Settler* s = dynamic_cast<Settler*>(units[coordinator.a_u_id]))
        {
            if (!s->isAuto())
            {
                // If the settler is standing on a good spot, build the city here.
                // Otherwise walk to the closest good spot on this landmass.
                if (isGoodCitySpot(s->latitude, s->longitude) && s->canBuildCity())
                {
                    CommandOrder co;
                    co.command = Command::BuildCityOrder;
                    coordinator.push(co);
                }
                else
                {
                    bool found = false;
                    coordinate spot = findCitySpot(s->getCoordinate(), found);

                    if (found)
                    {
                        s->goTo(spot.lat, spot.lon);
                    }
                    else
                    {
                        // No place left to settle on this landmass: disband the settler.
                        // The DisbandUnitOrder handler also advances coordinator.a_u_id to the
                        // next movable unit; without it the AI kept re-processing this settler
                        // every tick (blinking) until the user pressed a movement key.
                        s->availablemoves = 0;
                        printf("Settler %d (faction %d) disbanded: no good city spot left on this landmass.\n", s->id, s->faction);

                        CommandOrder co;
                        co.command = Command::DisbandUnitOrder;
                        coordinator.push(co);
                    }
                }
            }

        }
        else
        {
            // If the unit is already in a city, fortify it.
            City* nc = nullptr;
            if (Warrior* w = dynamic_cast<Warrior*>(units[coordinator.a_u_id]))
            for(auto& [cid,c]:cities)
            {
                if (c->faction == unit->faction && c->getCoordinate()==unit->getCoordinate())
                {
                    nc = c;
                    CommandOrder co;
                    co.command = Command::FortifyUnitOrder;
                    coordinator.push(co);
                }
            }

            if (Archer* w = dynamic_cast<Archer*>(units[coordinator.a_u_id]))
            for(auto& [cid,c]:cities)
            {
                if (c->faction == unit->faction && c->getCoordinate()==unit->getCoordinate())
                {
                    nc = c;
                    CommandOrder co;
                    co.command = Command::FortifyUnitOrder;
                    coordinator.push(co);
                }
            }

            // Pick a target only when the unit does not have one already (isAuto),
            // and pick the CLOSEST reachable enemy with a single Dijkstra run:
            // one goTo per enemy unit on every tick froze the game (sub-1 FPS) as
            // soon as a military unit was active with many units alive.
            if (Swordman* w = dynamic_cast<Swordman*>(units[coordinator.a_u_id]))
            if (!unit->isAuto())
            {
                // Attack the closest enemy unit (not the dying ones: corpses stay
                // in the units map until their death animation completes).
                std::vector<coordinate> enemies;
                for(auto& [uid,u]:units)
                    if (u->faction != unit->faction && !u->isDying())
                        enemies.push_back(u->getCoordinate());

                goToNearest(unit, enemies);
            }

            if (Horseman* w = dynamic_cast<Horseman*>(units[coordinator.a_u_id]))
            if (!unit->isAuto())
            {
                // Attack the closest enemy unit (not the dying ones: corpses stay
                // in the units map until their death animation completes).
                std::vector<coordinate> enemies;
                for(auto& [uid,u]:units)
                    if (u->faction != unit->faction && !u->isDying())
                        enemies.push_back(u->getCoordinate());

                goToNearest(unit, enemies);
            }

            if (nc == nullptr && !unit->isAuto())
            {
                // If there is a defenseless enemy city, capture the closest one (single Dijkstra).
                std::vector<coordinate> opencities;
                for(auto& [cid,c]:cities)
                {
                    if (c->faction != unit->faction && !c->isDefendedCity() && war)
                        opencities.push_back(c->getCoordinate());
                }

                if (!goToNearest(unit, opencities))
                {
                    // Boludeo
                    controller.registers.roll = getRandomInteger(-1.0,1.0);
                    controller.registers.pitch = getRandomInteger(-1.0,1.0);

                    if (controller.registers.roll==0 && controller.registers.pitch==0)
                    {
                        unit->availablemoves = 0;
                    }
                }
            }
        }

    }    
}

void autoPlayerCities()
{
    // Control city production.
    for(auto& [k,c]:cities)
    {
        if (c->faction == coordinator.a_f_id)
        {
            if (c->productionQueue.size()==0)
            {
                if (c->pop>1)
                {
                    // Populate the world first: as long as there is room for a new city
                    // on this city's landmass, keep building settlers.
                    bool found = false;
                    findCitySpot(c->getCoordinate(), found);

                    if (found && getNumberOfCities(c->faction)<50)
                    {
                        c->productionQueue.push(new SettlerFactory());
                    }
                    else
                    {
                        int rand = getRandomInteger(0,3);
                        switch (rand)
                        {
                            case 0:
                                c->productionQueue.push(new WarriorFactory());
                                break;
                            case 1:
                                c->productionQueue.push(new HorsemanFactory());
                                break;
                            case 2:
                                c->productionQueue.push(new SwordmanFactory());
                                break;
                            default:
                                c->productionQueue.push(new ArcherFactory());
                                break;
                        }
                    }
                }
            }
        }
    }    
}

void autoPlayer()
{
    autoPlayerMoveUnits();

    autoPlayerCities();
}