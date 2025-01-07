//  TestCase_001.cpp
//  bunmei
//
//  Created by Rodrigo Ramele on 07/09/2021
//

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

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../map.h"
#include "../units/Settler.h"
#include "../engine.h"
#include "../tiles.h"

struct Co {
    int lat;
    int lon;

    int pred;

    int dist;

};


struct Vertex {
    double m_d  = 0;
    size_t m_id = -1;
    // or std::size_t id() const;
};

struct Edge {
    double cost = 0;

    double getWeight(int i) const
    {
        return i*cost;
    }
};

using Graph =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Vertex, Edge>;

using boost::make_iterator_range;

using Tree = 
    boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Co, Edge>; // vectors for vertex and edges, bidirectional graph, vertex property and edge property

#include "TestCase_001.h"

TestCase_001::TestCase_001()
{

}

TestCase_001::~TestCase_001()
{

}

int TestCase_001::number()
{
    return 0;
}

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;

void TestCase_001::init()
{

    map.init();

    initTiles();

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(1);
        }


    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(1,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(2,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(3,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(4,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(5,0,"assets/assets/city/culture.png","Culture"));

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    
    factions.push_back(faction);

    for (auto& f: factions)
    {
        std::vector<coordinate> list;
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for(int lon=map.minlon;lon<map.maxlon;lon++)
            {
                if (map(lat,lon).code==1)
                {
                    list.push_back(coordinate(lat,lon));
                }
            }

        coordinate c(0,0);
        if (list.size()>0)
        {
            int r = getRandomInteger(0,list.size());
            c = list[r];
        }

        Settler *settler = new Settler();
        settler->longitude = c.lon;
        settler->latitude = c.lat;
        settler->id = getNextUnitId();
        settler->faction = f->id;
        settler->availablemoves = settler->getUnitMoves();

        units[settler->id] = settler;

    }

    citynames[0] = std::queue<std::string>();       // Vikings

    citynames[0].push("Kattegate");
    citynames[0].push("Jorvik");
    citynames[0].push("Hedeby");
    citynames[0].push("Trondheim");
    citynames[0].push("Bergen");
    citynames[0].push("Stavanger");
    citynames[0].push("Kristiansand");
    citynames[0].push("Oslo");
    citynames[0].push("Stockholm");
    citynames[0].push("Copenhagen");
    citynames[0].push("Helsinki");
    citynames[0].push("Reykjavik");

    /** 
    Graph g;
    auto v0 = add_vertex({0.1, 100}, g);
    auto v1 = add_vertex({0.2, 200}, g);
    auto v2 = add_vertex({0.3, 300}, g);
    auto v3 = add_vertex({0.4, 400}, g);
    auto v4 = add_vertex({0.5, 500}, g);
    auto v5 = add_vertex({0.6, 600}, g);

    add_edge(v0, v2, Edge{1.5}, g);
    add_edge(v1, v3, Edge{2.5}, g);
    add_edge(v4, v1, Edge{3.5}, g);
    add_edge(v2, v5, Edge{4.5}, g);

    auto idmap = boost::get(&Vertex::m_id, g);
    auto cost  = boost::get(&Edge::cost, g);

    auto find_by_id = [&g](size_t id) -> Vertex& {
        auto vv = boost::make_iterator_range(vertices(g));
        auto vd = find_if(vv, [&, id](auto vd) { return g[vd].m_id == id; });
        return g[*vd];
    };

    print_graph(g, idmap, std::cout << "original: ");

    auto i_want = [](auto vd) {
        return (vd % 2); // when I want
    };

    for (auto vd : make_iterator_range(vertices(g))) {
        if (i_want(vd))
            g[vd].m_id += 1;
        if (i_want(vd))
            idmap[vd] += 1;
        //put(idmap, vd, 42);
        //get(boost::vertex_bundle, g, vd).m_id = 999;
    }

    print_graph(g, idmap, std::cout << "altered: ");

    clear_vertex(v3, g);
    remove_vertex(v3, g); // undefined behaviour unless edges cleared

    print_graph(g, idmap, std::cout << "removed: ");

    for (auto ed : make_iterator_range(edges(g))) {
        std::cout << ed << " cost " << cost[ed] << "\n";
    }

    for (auto ed : make_iterator_range(edges(g))) {
        cost[ed] *= 111;
    }

    for (auto ed : make_iterator_range(edges(g))) {
        std::cout << ed << " cost " << cost[ed] << "\n";
    }**/




    Tree tree;
    auto root = add_vertex({15,10}, tree);
    auto north = add_vertex({10,10}, tree);
    auto south = add_vertex({20,10}, tree);

    auto f = add_vertex({100,100}, tree);

    add_edge(root, north, Edge{100}, tree);
    add_edge(root, south, Edge{200}, tree);
    add_edge(north, f, Edge{300}, tree);


    // cool, i now have the tree

    auto vpair = vertices(tree);
    for(auto iter=vpair.first; iter!=vpair.second; iter++)
    {
        auto vd = *iter;
        std::cout << "Vertex " << vd << " has lat " << tree[vd].lat << " and lon " << tree[vd].lon << std::endl;
    }

    auto epair = edges(tree);
    for(auto iter=epair.first; iter!=epair.second; iter++)
    {
        auto ed = *iter;
        std::cout << "Edge " << ed << " has cost " << tree[ed].cost << std::endl;
    }



    dijkstra_shortest_paths(tree, root, predecessor_map( get(&Co::pred, tree)).weight_map(get(&Edge::cost, tree)).distance_map(get(&Co::dist, tree)));


    vpair = vertices(tree);
    for(auto iter=vpair.first; iter!=vpair.second; iter++)
    {
        auto vd = *iter;
        std::cout << "Vertex " << vd << " has lat " << tree[vd].lat << " and lon " << tree[vd].lon << " and " << tree[vd].pred << std::endl;
    }




    auto co = boost::get(&Co::lat, tree);
    auto lo = boost::get(&Co::lon, tree);

    auto find_by_lat = [&tree](int lat) -> Co& {
        auto vv = boost::make_iterator_range(vertices(tree));
        auto vd = find_if(vv, [&, lat](auto vd) { return tree[vd].lat == lat; });
        return tree[*vd];
    };

    print_graph(tree, co, std::cout << "original: ");


}

int TestCase_001::check(int year)
{

    using namespace std;
    using namespace boost;

    /* define the graph type
          listS: selects the STL list container to store 
                 the OutEdge list
          vecS: selects the STL vector container to store 
                the vertices
          directedS: selects directed edges
    */
   typedef adjacency_list< listS, vecS, directedS > digraph;

   // instantiate a digraph object with 8 vertices
   digraph g(8);

   // add some edges
   add_edge(0, 1, g);
   add_edge(1, 5, g);
   add_edge(5, 6, g);
   add_edge(2, 3, g);
   add_edge(2, 4, g);
   add_edge(3, 5, g);
   add_edge(4, 5, g);
   add_edge(5, 7, g);

    //isdone = true;


    return 0;
}
std::string TestCase_001::title()
{
    return std::string("Generic Test Case");

}

bool TestCase_001::done()
{
    return isdone;
}
bool TestCase_001::passed()
{
    return false;
}
std::string TestCase_001::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_001();
}