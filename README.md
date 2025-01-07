# Bunmei

This is a remake of a classical game with all the stuf that I wanted to have it in the same game.  

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
