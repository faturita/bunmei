# Bunmei

This is a remake of a classical game with all the stuf that I wanted to have it in the same game.  

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


STK
---

First you need to copy the stk file from dependencies into the parent directory where you cloned wakuseiboukan.  Then you need
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
./b2 headers
```

## Compiling

```bash
make 
```

 # Running

```bash
./bunmei [-nointro] [-seed seed]
```

That's all folks.

# Game logic

## Map
Map size is 72x48.  Latitud determines the relation to the equator, increasing towards south, and longitud are the meridians increasing towards east.  Zero, Zero, is the center of the scren.  But, the map can be shifted sideways on the screen, moving the zero,zero position. The northern and sourthern part of the map work like in a Oblate Spheroid, so going north from longitud L at the top, means coming from the north at longitud -L, symmetric in relation to the zero meridian.  Of course when there is possible to move according to the unit.  This can change sea warfare.    

There are spetial resources around.

## Land Ownership

Land tiles belong to a faction and/or to a city.  When a unit moves into a tile, it owns the tile until it moves away from it.  Culture from cities generate ownership of tiles around them.  Culture spread ownership. And of course, armies can change that.


## Combat

The key to combat is experience.  Units can get experience by training.  Terrain plays a very important role.  Units have attack points, defense points and experience points.


## Resources
* There are six resources: food, shields, trade, coins, science, and culture.


# Features

* Arriving to the south and north pole (0,-24) (0,23).
* Circumvent the world.
* Ability to name landmarks, rivers, mountains, continents.
* Buy and sell resources from ports (like in Colonization).  This generates trade routes.
* Allow to form alliances with other factions and independence to arise from colonies.
* Deal with the problem of founding cities everywhere on the map: when cities are very close to each other, the one that produces more culture absorbs the other city, and one of them is destroyed, or refounded in the middle.
* Increase traderoutes: cities that create wagons or ships if they end up selling or buying something from other cities, automatically trade routes are established, that gives to the building (or supporting city) more trade resources.  This eliminates caravans.

## More info and Resources
* https://forums.civfanatics.com/threads/civ1-map-generation-explained.498630/
* https://civilization.fandom.com/wiki/Terrain_(Civ1)
* https://civilization.fandom.com/wiki/List_of_units_in_Civ4
* https://artlist.io/
* https://github.com/boostorg/graph
