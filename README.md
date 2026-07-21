# Bunmei 文明の旅行

This is a remake of a classical game with all the stuf that I wanted to have it in the same game.  

<img width="1025" alt="bunmeiscreenshot" src="https://github.com/user-attachments/assets/e720a91c-9380-4297-941f-a722696e63c8" />


# Compiling and Installation

## Prereqs on Mac

Install homebrew for Mac and run:

```bash
brew install premake
```

## Prereqs on Linux

**Tested on Ubuntu 20.02.4**

```bash
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install libbsd-dev freeglut3-dev libasound2 libasound2-dev
sudo apt-get install git make gcc g++
sudo apt-get install python3-pip
 ```


## Requirements

* OpenGL Version 2.1 (supported natively in Linux, Mac and Windows)
* STK: The Synthesis Toolkit in C++ (Audio Library)
* C++ Boost Library (particularly the Graph Library https://github.com/boostorg/graph)
* LodePNG https://github.com/lvandeve/lodepng (Bundled internally within this codebase).


STK
---

First you need to copy the stk file from dependencies into the parent directory where you cloned the project.  Then you need
to compile this sound library.

```bash
cp dependencies/stk.tgz ../../
cd ../../
tar xvzf stk.tgz
cd stk
make clean
./configure
make 
sudo make install
```

The STK libraries are going to be copied into /usr/local/lib/.  So you need
to run the following command before executing the simulator or configure it intorc_bash or similar.

```bash
export LD_LIBRARY_PATH=/usr/local/lib/
``` 

Boost
----- 

```
git clone https://github.com/boostorg/boost
cd boost
git submodule update --init
./bootstrap.sh
./b2 headers
```

## Compiling

```bash
make 
```

 # Running

```bash
./bunmei [-nointro] [-seed seed] [-mapsize size]
```

That's all folks.

# Game logic

## Map
The standard map size is 72x48. 

| Map Size | Effective Size | Default Zoom Level |
|---|---|---|
| 1 | 72x48 | 1.0 |
| 2 | 144x96 | 0.5 |
| 3 | 288x192 | 0.25 |
| 4 | 576x384 | 0.125 |
| 5 | 1152x768 | 0.0625 |

Latitud determines the relation to the equator, increasing towards south. Longitud are the meridians, increasing towards east.  (Zero, Zero) is the center of the scren.  But, the map can be shifted sideways on the screen, moving the zero,zero position. The northern and sourthern part of the map work like in a Oblate Spheroid, so going north from longitud L at the top, means coming from the north at longitud -L, symmetric in relation to the zero meridian.  Of course as long as is possible to make the movement according to the unit.  This can change sea warfare.  The map generation system depends on the size map and currently the generation procedure is very rough.

## Diplomacy and Wars

Civs have a relation between each other which goes from 1 to 8 (check table).  Indeed, it is a transition graph, a finite state machine.  Each state has a property which identifies the seizure or not of land: this means, what happen when a unit moves into a tile which belongs to the other, if it flips or not the ownership.  So all the factions are in a star configuration against all the other and their relations with each one of them is governed by a transition graph.


| DefCon ID | Relation Status | Land Seizure | Open Borders |
|---|---|---|---|
| 0 | No contact | Yes | Yes |
| 1 | Foe | Yes | Yes |
| 2 | Ceasefire | No | No |
| 3 | Armistice | No | No |
| 4 | Peace | No | No |
| 5 | Non Agression Agreement | No | No |
| 6 | Trade Agreement | No | Yes |
| 7 | Coalition | No | Yes |
| 8 | Vassalage | No | Yes |

So, 
- **Open Borders**: this flag determines if the movement can be executed or not, i.e. if the unit invading a foreign land tile (owned by a another faction) if that can be done or not.  If the flag is false it is a place where the unit cannot move and should be excluded from the tree traversal algorithms.
- **Land Seizure**: this flag determines what happen when an enemy unit moves into an enemy tile.  If this flag is true, it means that the invading unit captures the tile and change the ownership.  So a city will loose access to this working tile and should assign a different one.


## Land Ownership

Land tiles belong to a faction and/or to a city.  When a unit moves into a tile, it owns the tile until it moves away from it.  However, units execute this land seizure when they are at war with other civilizations (it depends on the diplomacy between them).  Culture from cities generate ownership of tiles around them.  Culture spread ownership. And of course, armies can change that.  At the same time, armies need Line of Sight from their cities to keep fighting.  And this is determined by tile ownership rules.  Land ownership allows to access resources on the map, and it is also required in combat.

## Resources
There are special resources around. 

There are six basic resources: food, shields, trade, coins, science, and culture.  Special resources change tile production but at the same time they are accumulated in nearby cities and can be loaded into ships.  Access to resources depend on the LoS to the resource itself and requires a road to it.

Tradable resourcees are:

| Resource |   |  |
|---|---|---|
| Tobacco | 72x48 | 1.0 |
| Cotton | 144x96 | 0.5 |
| Sugar | 288x192 | 0.25 |
| Furs | 576x384 | 0.125 |
| Lumber | 1152x768 | 0.0625 |
| Silver | 576x384 | 0.125 |
| Gold | 1152x768 | 0.0625 |
| Iron | 576x384 | 0.125 |
| Copper | 1152x768 | 0.0625 |
| Carbon | 576x384 | 0.125 |
| Uranium | 1152x768 | 0.0625 |
| Oil | 576x384 | 0.125 |
| Lumber | 1152x768 | 0.0625 |

## Trade

Food and all the special resources can be directly traded, by loading them into boats and shipping them to foreign cities in exchange of money or other resources.


## Population

Population represents humans, so they are handled like human population.  So the basic rule of increase population when food storage is completed, there is a logistic growth equation that is proportional to current population, according to food availability.  Settlers are moving population, the same as workers.  Workers work faster according to their size.  This also opens the possibility of indentured servants, slaves and so on.

## Science

Instead of selecting what scientific advance do you seek, you can invest research points into things that you already know.  The science tree connects all the different discoveries with a sigmoid weighted linear activation.  For instance, investing in Alphabet and Mathematics may lead to the discovery of Astronomy.  But it doesn't really make sense to invest in what you already do not know (like other games like this do).  So the idea is have like a multilayered neural network where the each node correspond to a concrete scientific discovery and the sigmoid function depends on weights that are increased as long as they are invested.  This will produce the triggering of each one of the discoveries, which now will look more random and it will depend on what do you invest.  

## Combat

The key to combat is experience.  Units can get experience by training.  Terrain plays a very important role.  Units have attack,defense, terrain and city weights.  Terrain and city are bilinear with additional weights that depend on the terrain itself and on the city.  Each unit have headcounts or soldiers that determine their size.  Each battle, soldiers inevitable die (this is the stochastic component) according to the power balance.  So the unit is weakened in discrete steps.  A Roman Legion had 6k soldiers. Other units have also a number that represent how many soldiers the unit have. The unit needs to gather population from somewhere to increase their numbers !   So units have a pop number (and this make sense with workers and settlers).



# Working issues

* Ui for investing in science.
* <strike>Allow GoTo with naval units.</strike>
* <strike>Replace controllingid with activeid.</strike>
* Iterate on improving the AI: focus more on production on cities and build defensive units.
* Show units in cityscreen.
* Add a way in which fortified units can be selected.
* Sentry is clearly used only for ships and I think that is Ok.
* Allow ships to be shown last:  perhaps it is better to add a number, like a Z-value that will help to determine what is shown first and what is shown last.
* <strike>Paint land ownership.  This can be easily done using the png tile that shows red on each city based on the f_owner_id value.</strike>
* The f_owner_id value can be used as temperature on a heat diffusion model.  The heat can be increased by culture.  When a unit visit a tile, it own the tile, but when the unit left, the sourrounding heat will be transferred by the gaussian smoothing operation.
* Add the spheroid concept into map: by doing this we can make sense of the maps locations on the south and north (what if the offset procedure can be extended vertically as well?)
* Add a world edit testcase to create customized maps.
* Include diplomatic into Factions (and remove the war bool flag).
* <strike>Improve the way we can center into cities: this will help to fix the issue with the city screen.</strike>
* Handle what happens when a trireme lands into a occupied land, a city land, or a land with enemy units (amphibious attack?).
* <strike>Show attack unit versus defending units.</strike>

# Features

* Arriving to the south and north pole (0,-24) (0,23).
* Circumvent the world.
* Ability to name landmarks, rivers, mountains, continents.
* Buy and sell resources from ports (like in Colonization).  This generates trade routes.
* Allow to form alliances with other factions and independence to arise from colonies.
* Deal with the problem of founding cities everywhere on the map: when cities are very close to each other, the one that produces more culture absorbs the other city, and one of them is destroyed, or refounded in the middle.
* Increase traderoutes: cities that create wagons or ships if they end up selling or buying something from other cities, automatically trade routes are established, that gives to the building (or supporting city) more trade resources.  This eliminates caravans.
* Population now represent head count, humans, people.  
* Each city has a exponential growth model that allows the **population** to increase based on current population and the availability of food.  * The **pop** value on each city is that value quantified.  Food existence allows population to growth. 
* Units also have **soldiers**.  
* Settlers are moving population, and the city that is built starts with the number of citizens.  
* Same for workers, their strength is derived from the size of the group.
* **Units**: members determine a stepped factor for the fighting equation.  The shape of this function depends on the unit.
* **Units**: the **personnel** factor is stepped.
* Hence, in battles, people die.  The stochastic factor determines how many casualties each unit have on each battle.
* Units require access to population to recover from battles.
* Units also require **Line Of Sight** to the capital city, by having a continuos line of access (or free land in the middle).  
* Roads keep ownership.  This allows to increase the line of sight regardless of culture.
* Food can be shipped.  This allows to cities without any harvesting to increase if they manage to import food.
* Cities can upgrade city walls to Forts and Fortress.  These can open fire on naval units in nearby tiles.
* Air units can move freely but within their airrange.  They need airfields to refuel.  They can hope in the same turn from city to city.
* There are also airfields which are isolated fort upgrades.
* Bombers can attack cities and enemy units.
* Carrier can transport air units.
* EVALUATE asynchronous movement.


## More info and Resources
* https://forums.civfanatics.com/threads/civ1-map-generation-explained.498630/
* https://civilization.fandom.com/wiki/Terrain_(Civ1)
* https://civilization.fandom.com/wiki/List_of_units_in_Civ4
* https://artlist.io/
* https://craftpix.net/
* https://www.storyblocks.com/
* https://github.com/boostorg/graph
* https://github.com/rajko-horvat/OpenCiv1
* https://github.com/SWY1985/CivOne

