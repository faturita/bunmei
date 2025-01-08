#include "ai.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <deque>
#include <iterator>

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

#include <boost/bind.hpp>

#include <boost/phoenix.hpp>
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


// Build the tree and the path to the destination.
// Calculate the next movement based on Dijkstra algorithm.
// @FIXME: Only works for land, it should consider water, the terrain cost, and also the presence of enemy units and cities.
coordinate goTo(Unit* unit, bool &ok)
{

    Tree tree;

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for(int lon=map.minlon;lon<map.maxlon;lon++)
        {
            if (map.peek(lat,lon).code == LAND)
                auto v = add_vertex({lat,lon,0,0}, tree);
        }

    auto vv = boost::make_iterator_range(vertices(tree));

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
                            add_edge(*start, *end, Edge{1}, tree);
                        }
                    }
            }
        }

    
    // At this point, the tree is built.

    int lat = unit->latitude;
    int lon = unit->longitude;
    auto source = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    printf("Start %d %d\n", lat, lon);

    dijkstra_shortest_paths(tree, *source, predecessor_map( get(&CoordinateVertex::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&CoordinateVertex::dist, tree)));
            

    // Track back the result until you find where the unit is located now.
    lat = unit->target.lat;
    lon = unit->target.lon;
    auto target = find_if(vv, [&, lat,lon](auto vd) { return tree[vd].lat == lat && tree[vd].lon == lon; });

    int lasttarget=-1;
    while(tree[*target].pred != *target)
    {
        printf("Target %d: %d,%d Predecessor %d\n",*target, lat,lon,tree[*target].pred);
        int pred = tree[*target].pred;
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
        printf("There is no way to get there...\n");
        return coordinate(0,0);
    }

    ok = true;
    printf("Next movement: %d %d %d\n",lasttarget,tree[lasttarget].lat, tree[lasttarget].lon);
    return coordinate(tree[lasttarget].lat, tree[lasttarget].lon);

}

