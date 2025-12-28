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


Tree buildTree()
{
    Tree tree;

    // Add all the land vertices into the graph.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for(int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (map.peek(lat,lon).code == LAND)
                auto v = add_vertex({lat,lon,0,0}, tree);
        }

    auto vv = boost::make_iterator_range(vertices(tree));

    // Now, map all the connections between land mapcells.  This allow a unit to move from one land cell to another.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for(int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (map.peek(lat,lon).code == LAND)
            {
                auto start = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });
                
                for(int i=-1;i<=1;i++)
                    for(int j=-1;j<=1;j++)
                    {
                        if (i==0 && j==0)
                            continue;

                        if (map.peek(lat+i,lon+j).code == LAND)
                        {
                            auto end = find_if(vv, [&, lat,lon,i,j](auto vd) { return tree[vd].lat == lat+i && tree[vd].lon == lon+j; });
                            // @FIXME: Add terrain cost here on the edge.
                            add_edge(*start, *end, Edge{1}, tree);
                        }
                    }
            }
        }

    return tree;
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

                        if (map.peek(lat+i,lon+j).code == LAND && 
                        (
                        map.peek(lat+i,lon+j).isUnassignedLand() || 
                        map.peek(lat+i,lon+j).getOwnedBy() == faction || 
                        (map.peek(lat+i,lon+j).getOwnedBy() != faction && war)
                        ) )
                        {
                            auto end = find_if(vv, [&, lat,lon,i,j](auto vd) { return tree[vd].lat == lat+i && tree[vd].lon == lon+j; });
                            // @FIXME: Add terrain cost here on the edge.
                            add_edge(*start, *end, Edge{1}, tree);
                        }
                    }
            }
        }

    return tree;
}



coordinate reachableHorizon(Unit* unit, int jumpingdistance, int f_id, bool &ok)
{
    Tree tree = buildTraversalTree(f_id);
    auto vv = boost::make_iterator_range(vertices(tree));
    

    auto vpair = vertices(tree);
    for(auto iter=vpair.first; iter!=vpair.second; iter++)
    {
        auto vd = *iter;
        std::cout << "Vertex " << vd << " has lat " << tree[vd].lat << " and lon " << tree[vd].lon << " Predecessor: " << tree[vd].pred << std::endl;
    }

    auto vedges = edges(tree);
    for(auto iter=vedges.first; iter!=vedges.second; iter++)
    {
        auto ed = *iter;
        std::cout << "Edge " << ed << " has cost " << tree[ed].cost << std::endl;
    }
    // At this point, the tree is built.

    int lat = unit->latitude;
    int lon = unit->longitude;
    auto source = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    printf("Start %d %d\n", lat, lon);

    dijkstra_shortest_paths(tree, *source, predecessor_map( get(&CoordinateVertex::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&CoordinateVertex::dist, tree)));

    int vertexid = *source;
    auto nextStep = find_if(vv, [&, vertexid](auto vd) { return tree[vd].pred == vertexid; });
    
    // This can be nonexistant
    if (nextStep == vv.end())
    {
        ok = false;
        return coordinate(0,0);
    }

    // Move forward towards somewhere
    int jumps = 1;
    while(jumps < jumpingdistance)
    {
        printf("Next to %d: %d,%d\n",*nextStep, lat,lon);
        vertexid = *nextStep;
        nextStep = find_if(vv, [&, vertexid](auto vd) { return tree[vd].pred == vertexid; });

        if (nextStep == vv.end())
        {
            ok = false;
            return coordinate(0,0);
        }
        jumps++;
    }

    ok = true;
    return coordinate(tree[vertexid].lat, tree[vertexid].lon);
        
}


// Build the tree and the path to the destination.
// Calculate the next movement based on Dijkstra algorithm.
// @FIXME: Only works for land, it should consider water, the terrain cost, and also the presence of enemy units and cities.
int reachableLand(Unit* unit, bool &ok)
{

    Tree tree = buildTree();
    auto vv = boost::make_iterator_range(vertices(tree));
    
    // At this point, the tree is built.

    int lat = unit->latitude;
    int lon = unit->longitude;
    auto source = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    printf("Start %d %d\n", lat, lon);

    dijkstra_shortest_paths(tree, *source, predecessor_map( get(&CoordinateVertex::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&CoordinateVertex::dist, tree)));
            
    auto vpair = vertices(tree);
    int reachableland=0;
    for(auto iter=vpair.first; iter!=vpair.second; iter++)
    {
        auto vd = *iter;
        reachableland++;
        //std::cout << "Vertex " << vd << " has lat " << tree[vd].lat << " and lon " << tree[vd].lon << " Predecessor: " << tree[vd].pred << std::endl;
    }

    return reachableland;
}


// Build the tree and the path to the destination.
// Calculate the next movement based on Dijkstra algorithm.
// @FIXME: Only works for land, it should consider water, the terrain cost, and also the presence of enemy units and cities.
coordinate goTo(Unit* unit, bool &ok, int targetlat, int targetlon)
{

    if (map.peek(unit->latitude,unit->longitude).code != LAND || (!(map.peek(unit->latitude,unit->longitude).isUnassignedLand() || map.peek(unit->latitude,unit->longitude).getOwnedBy() == unit->faction)))
    {
        ok = false;
        return coordinate(0,0);
    }


    Tree tree = buildTraversalTree(unit->faction);
    auto vv = boost::make_iterator_range(vertices(tree));
    
    // At this point, the tree is built.

    int lat = unit->latitude;
    int lon = unit->longitude;
    auto source = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    printf("Start %d %d\n", lat, lon);

    dijkstra_shortest_paths(tree, *source, predecessor_map( get(&CoordinateVertex::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&CoordinateVertex::dist, tree)));
            

    // Track back the result until you find where the unit is located now.
    lat = targetlat;
    lon = targetlon;
    auto target = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    int lasttarget=-1;
    while(tree[*target].pred != *target)
    {
        printf("Target %d: %d,%d Predecessor %d\n",*target, lat,lon,tree[*target].pred);
        int pred = tree[*target].pred;

        // Check if prd is a valid element in the graph.
        if (pred < 0 || pred >= (int)num_vertices(tree))
        {
            ok = false;
            printf("There is no way to get there...\n");
            exit(-1);
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
        std::cout << "Vertex " << vd << " has lat " << tree[vd].lat << " and lon " << tree[vd].lon << " Predecessor: " << tree[vd].pred << std::endl;
    }

    if (lasttarget == -1)
    {
        ok = false;
        printf("There is no way to get there...\n");
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

#include "Faction.h"
#include "usercontrols.h"
#include "coordinator.h"
#include "units/Warrior.h"
#include "units/Settler.h"
#include "units/Horseman.h"
#include "units/Archer.h"


extern Coordinator coordinator;
extern Controller controller;

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

void autoPlayer()
{
    if (units.find(coordinator.a_u_id)!=units.end())
    {
        Unit *unit = units[coordinator.a_u_id];
        if(Settler* s = dynamic_cast<Settler*>(units[coordinator.a_u_id]))
        {
            if (!s->isAuto())
            {
                // Find the nearest city, and move AWAY from it as far as possible, 
                //    and if there is no city, build it here.
                City* nc = nullptr;
                float distance = 6;
                for(auto& [cid,c]:cities)
                {
                    float d = (Vec3f(s->latitude,0,s->longitude)-Vec3f(c->latitude,0,c->longitude)).magnitudeSquared();
                    if (d<distance)
                    {
                        distance = d;
                        nc = c;
                    }
                }

                if (nc!=nullptr)
                {
                    Vec3f rad = getRandomCircularSpot(Vec3f(s->latitude,0,s->longitude),6);

                    while (map.peek(rad[0],rad[2]).code != LAND || !map.peek(rad[0],rad[2]).isUnassignedLand())
                    {
                        rad = getRandomCircularSpot(Vec3f(s->latitude,0,s->longitude),6);
                    }

                    s->goTo(rad[0],rad[2]);

                    
                }
                else
                {
                    if (s->canBuildCity())
                    {
                        CommandOrder co;
                        co.command = Command::BuildCityOrder;
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

            if (Horseman* w = dynamic_cast<Horseman*>(units[coordinator.a_u_id]))
            for(auto& [uid,u]:units)
            {
                // Attack unit.
                if (u->faction != unit->faction)
                {
                    unit->goTo(u->latitude,u->longitude);
                }
            }

            if (nc == nullptr)
            {
                // If there is a defenseless enemy city nearby, capture it.
                City* cc = nullptr;
                for(auto& [cid,c]:cities)
                {
                    if (c->faction != unit->faction && !c->isDefendedCity() && war)
                    {
                        bool ok;
                        coordinate co = goTo(unit, ok, c->latitude, c->longitude);
                        if (ok)
                            cc = c;
                    }
                }

                if (cc == nullptr)
                {
                    // Boludeo
                    controller.registers.roll = getRandomInteger(-1.0,1.0);
                    controller.registers.pitch = getRandomInteger(-1.0,1.0);

                    if (controller.registers.roll==0 && controller.registers.pitch==0)
                    {
                        unit->availablemoves = 0;
                    }
                } else {
                    unit->goTo(cc->latitude,cc->longitude);
                }
            }
        }

    }

    // Control city production.
    for(auto& [k,c]:cities)
    {
        if (c->faction == coordinator.a_f_id)
        {
            if (c->productionQueue.size()==0)
            {
                if (c->pop>1)
                {
                    int rand = getRandomInteger(0,3);

                    switch (rand) 
                    {
                        case 0:
                            c->productionQueue.push(new WarriorFactory());
                            break;
                        case 1:
                        {
                            int numberOfCities = getNumberOfCities(c->faction);

                            if (numberOfCities<5)
                            {
                                c->productionQueue.push(new SettlerFactory());
                            }
                        }
                            break;
                        case 2:
                            c->productionQueue.push(new HorsemanFactory());
                            break;
                        case 3:default:
                            c->productionQueue.push(new ArcherFactory());
                            break;
                    }
                }
            }
        }
    }
}

