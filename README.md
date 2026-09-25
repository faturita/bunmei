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
./bunmei [-nointro] [-seed seed] [-mapsize size] [-nofog] [-civs N] [-faction n]
```

That's all folks.

# Game logic

## Design Rules

* Every aspect of the game can be automated
* The game can be fully simulated (without graphics).
* The game mechanics are mediated through an intermediate language (command model)

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

## Units

| Unit | Dep Tech  | Commodity  |  |
|---|---|---|---|
| Settler         | -                 |  | - |
| Worker          | -    |  |  |
| Wagon           | Wheel      | - | - |
| Warrior         | -            | - | - |
| Scout           | Animal Husbandry     |  |  |
| Archer          | Archery           | - | - |
| Spearman        | Warrior Code                | -    | - |
| Swordman        | Iron Working           | Iron/Copper   | - |
| Pretorian       | Iron Working           | Iron/Copper | - |
| Axeman          | Iron Working          | Iron/Copper | - |
| Horseman        | Horseback Riding                | Horses    | - |
| Chariot         | Wheel                | Horses    | - |
| War Elephant.   | Animal Husbandry                | Elephants    | - |
| Trireme         | Map Making      | - | - |
| Galley          | Map Making + Horseback Riding  | -    | - |
| Horse Archer    | Animal Husbandry + Horseback Riding | -    | - |
| Galleon         | Ship Building | - | - |
| Musketman       | Gunpowder | Guns | - |
| Granadier       | Chemestry | Guns + Spices | - |
| Cavalry         | Military Tradition | Horses + Guns | - |
| Cannon          | Metallurgy | Iron |

Units should strip population from their city, and they could rejoin the city back.  Fortify them help them to recover their headcounts (soldiers) from a nearby city with LoS.  So all the units have a headcount field which determines the amount of soldiers or the amount of population that the unit has. So every unit can join back a city and increase city population according to the growth rate rule of population.

## Cities

| Buildings |  Dep Tech |  City Context Perks |  |
|---|---|---|---|
| Palace           | -                 | 0x01 | - |
| Barracks         | Warrior Code    | 0x02 |  |
| Granary          | Pottery         |  0x03| - |
| Market           | Currency            | 0x04 | - |
| Collosseum       | Writing            | 0x05 | - |
| Temple           | Ceremonial Burial  | 0x06 | - |
| University.      | Education.         | 0x07 | - |
| Theatre          | Music              | 0x08 | - |
| Stable           | Horseback Riding   | 0x09 | - |
| Warehouse        | Pottery.           | 0x0a | - |
| Observatory      | Astronomy          | 0x0c | |
| Monument         | Mysticism.         | | |
| Lighthouse       | Map Making.        | | |
| Library          | Writing.           | | |
| Harbor           |                    | | |
| Grocer           | Trade |||
| Forge            |.       |||
| Factory          | Industrialization       || iron/copper→tools|
| Depot            |        | 0x0b | 100 Tools |
| CourtHouse       |        | Reduces the value for Subsistant on the city||
| Cathedral.       |.       |||
| Castle.          |        | Defensive units have a boost in their terrain defense value||
| Fort             |.       | Allow reducing the number of moves of enemy units| |
| Fortress.        |.       | Allow firing and stopping enemy units that come around the sea city | |
| Bank             | Duplicates trade output in the city|||
| Aqueduct         | Allow irrigation on the city tile without nearby water and from there allows to irrigate neighbouring tiles|||
| Rolling House    | || tobacco→cigars|
| Processing Plant | || sugar→rum |
| Armory           | Gunpowder | | tools→guns |
| Textile Mill.    | Industrialization | | cotton/furs→textiles |
| Steelworks.      | Steel.    | | iron→steel |
| Assembly Plant   | Automobiles | | steel→automobile |
| Oil Refinery.    | Plastics | | |
| Pharma.          | Medicine | | |
| Semiconductor Fab| Electronics | | |
| Gigafactory.     | Robotics | | | 

* City perks for the dependency tree

| Perk |  Code |  Description |
|---|---|---|
| VETERAN_CODE                  | 0x02         |  Units created in the city have attack and defense increased by 100%.|
| HALF_POPULATION_CODE          | 0x03         |  The amount of food required for population growth is reduced by half.|
| TRADE_SURPLUS_CODE            | 0x04         |  Trade output in the city increases by 100%.|
| STORAGE_EXPANSION_1           | 0x0a         |  Storage is increased by 100%.|
| STORAGE_EXPANSION_2           | 0x0b         |  Storage is increased by 100%.|
| SCIENCE_SURPLUS_CODE          | 0x0c         |  Duplicates science output in the city.|

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

There are six basic core resources: food, shields, trade, coins, science, and culture.  

| Resource | 
|---|
| Food      | 
| Shields    | 
| Trade      | 
| Coins      | 
| Science    | 
| Culture    | 
| Luxury     |

Food allows population survive and growth.  Shields power production.  Trade represent commercial activities and it can be converted into Coins, Science, Culture and Luxury by the fundamental tax rate.  Coins are accumulated by the government on each city, Science goes for research funding and Culture promotes borders and landownership.  Luxury empowers population wealth and is a measure of how much power you can grab from your population to do stuff.

Special resources can appear on the map.  They can change tile production but at the same time they allow the production of commodities which can be accumulated in the city.  Cities can access special resources inside city tiles, without having to work on them,  and also can access other nearby special resources as long as the LoS is established and the faction owns the special resource tile.

The resources and their commodities are:

| Resource | Scientific Advancement  | Improvement on tile  |  Commodity |
|---|---|---|---|
| Gold      | -                 | Mine | - |
| Copper    | Bronze Working    | Mine | Copper |
| Iron      | Iron Working      | Mine | Iron |
| Silver    | Mining            | Mine | Silver |
| Marble    | Masonry           | Quarry | Marble |
| Fish      | -                 | -    | - |
| Doe       | Hunting           | Camp   | Furs |
| Game      | Hunting           | Camp | Furs |
| Seal      | Hunting.          | Camp | Furs |
| Whales    | -                 | -    | Traan |
| Oasis     | -                 | -    | - |
| Geoshield | -                 | -    | - |
| Gems      | Iron Working      | Mine | Gems |
| Horse     | Horseback Riding  | -    | Horses |
| Cattle    | Animal Husbrandry | -    | Meat |
| Elephants | Hunting           | -    | Elephants |
| Silk      | -                 | -    | Silk |
| Grapes    | Feudalism.        | Plantation | Wine |
| Spices    | -                 | -    | Spices |
| Gunpowder | Gunpowder         | -    | Gunpowder |
| Sugar     | -                 | Plantation | Sugar |
| Tobacco   | -                 | Plantation | Tobacco |
| Cotton    | -                 | Plantation | Cotton |
| Carbon    | Industrialization | Mine | Carbon |
| Uranium   | Radioactivity     | Mine | Uranium |
| Oil       | Refining          | Derrick | Oil |
| Litium    | Electronics.      | Mine | Litium |
| Aluminium | Composites.       | Mine | Aluminium |
| Helium-3  | Fussion           | Mine | Hellium-3 |

(some special resources do not produce any commodity)

Commodities are gathered in the cities without working their respective tiles.  So, the city will gather each turn the commodity produced by all the tiles with special resources around, that are free, belong to the city or to the faction.

Tiles with special resources owned by a faction by the presence of military forces are available to neaby cities by LoS.

## Manufactured goods

Some buildings take commodities and convert them into manufactured goods.  These are sold at much higher price and allow the increase in city building capacities (i.e. industrialization).  The list of manufactured goods are:

| Mfg Good (output) | Scientific Advancement | Building | Input materials |
|---|---|---|---|
| Cigars     | Something before factory | Rolling House | Tobacco |
| Rum.       | Ship Building     | Processing Plant | Sugar |
| Tools      | Ship Building     | Factory | Copper / Iron |
| Guns       | Gunpowder         | Armory  | Tools + Gunpowder |
| Textiles   | Industrialization | Textile Mill | Cotton / Furs |
| Steel      | Steel             | Steelworks   | Tools |
| Automobiles| Combustion        | Assembly Plant | Steel + Oil |
| Plastics   | Plastics          | Oil Refinery | Oil           |
| Pharmaceuticals | Medicine     | Pharma       | Spices / Game / Doe / Sugar |
| Electronics | Electronics      | Semiconductor Fab | Aluminium / Litium / Silver|
| Robots |  Robotics           | Gigafactory  | Electronics + Steel |

Perhaps I can add Laudanum ??? 

So the idea is that those who succeed are able to create the complex supply chains that in the end allow to create better armies.

This will probably lead to the next level which is services.

## Trade

All the commodities and manufactured goods can be directly traded, by loading them into boats or wagons and shipping them to foreign cities in exchange for money or other commodities.  'Wagons' appear early in the game (with Trading and Wheel) and allow to trade resources between land cities.  Selling or Buying stuff produces extra trade flow in the city. 

Some buildings and unit require special resources to be produced.

Resources are accumulated in each city, including coins.  Coins are used to maintain buildings and units.  Coins from the government are accumulated in the capital city and then resources are shared across all the cities.


## Population

Population represents humans, so they are handled like human population.  So the basic rule of increase population when food storage is completed, there is a logistic growth equation that is proportional to current population, according to food availability.  Settlers are moving population, the same as workers.  Workers work faster according to their size.  This also opens the possibility of indentured servants, slaves and so on.

## Science

Instead of selecting what scientific advance do you seek, you can invest research points into things that you already know.  The science tree connects all the different discoveries with a sigmoid weighted linear activation.  For instance, investing in Alphabet and Mathematics may lead to the discovery of Astronomy.  But it doesn't really make sense to invest in what you already do not know (like other games like this do).  So the idea is have like a multilayered neural network where the each node correspond to a concrete scientific discovery and the sigmoid function depends on weights that are increased as long as they are invested.  This will produce the triggering of each one of the discoveries, which now will look more random and it will depend on what do you invest.  

The discovery of a technology or other achievements enable unit productions or resource production.  Everything is mediated through the commands.

| Tech | Technology Code | Dependencies |
|---|---:|---|
| Language | `0x01` | - |
| Hunting | `0x02` | Language (1.0) |
| Agriculture | `0x03` | Hunting (1.0) |
| Fishing | `0x04` | Hunting (1.0) |
| Mining | `0x05` | Language (1.0) |
| Masonry | `0x06` | Mining (0.9)|
| The Wheel | `0x07` | Agriculture (1.0) |
| Archery | `0x08` | Fishing (0.8), Hunting (1.0) |
| Warrior Code | `0x09` | Archery (0.8), Hunting (1.0) |
| Bronze Working | `0x0A` | Mining (0.8), Hunting (1.0) |
| Animal Husbandry | `0x0B` | Agriculture (0.8), Hunting (1.0) |
| Pottery | `0x0C` | Masonry (1.0), Agriculture (1.0) |
| Alphabet | `0x0D` | Language (1.0) |
| Ceremonial Burial | `0x0E` | Warrior Code (1.0) |
| Writing | `0x0F` | Alphabet (1.0) |
| Mathematics | `0x10` | Masonry (1.0), Alphabet (1.0) |
| Iron Working | `0x11` | Bronze Working (1.0) |
| Horseback Riding | `0x12` | Warrior Code (1.0), Animal Husbandry (1.0), Archery (1.0) |
| Construction | `0x13` | Masonry (1.0), Iron Working (0.9), Mathematics (0.8), The Wheel (0.9) |
| Currency | `0x14` | Iron Working (1.0), Mathematics (1.0) |
| Mysticism | `0x15` | Ceremonial Burial (1.0) |
| Map Making | `0x16` | Fishing (1.0), Alphabet (1.0), Pottery (1.0) |
| Polytheism | `0x17` | Warrior Code (0.9), Mysticism (1.0) |
| Literature | `0x18` | Writing (1.0), Alphabet (0.8) |
| Code of Laws | `0x19` | Writing (1.0), Warrior Code (1.0)|
| Philosophy | `0x1A` | Mathematics (1.0), Writing (1.0) |
| Metal Casting | `0x1B` | Iron Working (0.9), Construction (1.0) |
| Monotheism | `0x1C` | Mysticism (0.9), Polytheism (1.0)|
| Republic | `0x1D` | Code of Laws (1.0), Philosophy (1.0) |
| Monarchy | `0x1E` | Polytheism (0.9), Monotheism (1.0)|
| Feudalism | `0x1F` | Archery (0.7), Monarchy (1.0), Currency (0.9) |
| Ship Building | `0x20` | Construction (1.0), Map Making (1.0), Metal Casting (1.0), Feudalism (1.0) |
| Theology | `0x21` | Monotheism (1.0), Philosophy (1.0) |
| Education | `0x22` | Alphabet (0.7), Literature (0.9), Republic (1.0), Theology (1.0) |
| Astronomy | `0x23` | Alphabet (0.7), Mathematics (0.8), Map Making (0.8), Ceremonial Burial (0.8), Ship Building (1.0), Education (1.0) |
| Banking | `0x24` | Currency (0.9), Code of Laws (0.9), Education (1.0) |
| Chivalry | `0x25` | Monotheism (0.9), Feudalism (1.0), Monarchy (1.0), Theology (0.9) |
| Physics | `0x26` | Alphabet (0.7), Mathematics (0.7), Iron Working (0.8), Astronomy (1.0)|
| Gunpowder | `0x27` | Pottery (0.6), Ceremonial Burial (0.5), Feudalism (1.0) |
| Magnetism | `0x28` | Map Making (0.6), Iron Working (0.6), Ship Building (0.9), Astronomy (1.0) |
| Chemistry | `0x29` | Ceremonial Burial (0.4), Pottery (0.4), Gunpowder (0.9), Physics (1.0) |
| Metallurgy | `0x2A` | Iron Working (0.5), Bronze Working (0.5), Metal Casting (0.8), Chemistry (1.0) |
| Charters | `0x2B` | Currency (0.5), Banking (0.7), Writing (0.4), Code of Laws (0.4), Philosophy (1.0) |
| Music | `0x2C` | Ceremonial Burial (0.4), Mathematics (0.2), Literature (0.3), Mysticism (0.5), Education (1.0) |
| Industrialization | `0x2D` | Charters (1.0), Magnetism (0.9), Code of Laws (0.8), Currency (0.5), Metallurgy (0.7) |
| Military Tradition | `0x2E` | Warrior Code (0.3), Horseback Riding (0.3), Literature (0.5), Chivalry (0.6), Education (0.7), Gunpowder (0.8), Chemistry (0.9) |

## Government and Society

| Government | Historical archetype |
|---|---|
| **Tribalism** | Kinship societies, clans, and tribal confederations |
| **Despotism** | Ancient autocracies and absolute personal rule |
| **Monarchy** | Hereditary kingdoms and dynastic states |
| **Republic** | Citizen or aristocratic republics, such as Rome and Venice |
| **Democracy** | Popular government and broad civic participation |
| **Theocracy** | Political authority legitimized and controlled by religion |
| **Communism** | Soviet-style one-party state and centrally planned economy |
| **Liberal** | Modern Western liberal democracies and capitalist market economies |
| **Fascism** | Nationalist, authoritarian, militarized state |
| **Renminism** | Chinese-style one-party state combined with a market economy and strong state direction |
| **Technofeudalism** | Future society dominated by technology platforms, AI, and concentrated economic power |

So these government type affect economy, society, production and so on.

### Slaves

There is a unit called 'Sklave' (in german) which represents slaves captured from capturing enemy units.  They can be added into a city and put to work on tiles without consuming food (free labor).

### Religion

Religion is an amazing aspect of society.

| Religion | Origin / Tradition | Tech |
|---|---|
| **Dingir** | Ancient Mesopotamian polytheism |
| **Hinduism** | Ancient India |
| **Buddhism** | Ancient India |
| **Judaism** | Ancient Levant |
| **Christianity** | Roman/Levantine world |
| **Islam** | Arabia |
| **Confucianism** | Ancient China |

Religion monuments derive luxury points.
Culture is spread easily within the same religion boundaries.
Diplomacy is easier and follow the path of religion

### Culture

Culture is generated on each city and works like the heat diffusion.  So cities are heat sources and the tiles work spreading 
the culture and getting cold.  But the model allows several different types of heat to spread and depending on the heat type
that will determine the tile natural ownership.

So if normalized temperature[faction] > 0.5, then tile.f_id = faction.  The faction owns the tile even if there is no army there.   Armies naturally override that.

```
If a city->temperature[faction] > 0.5 and city->unhappy > 0.5 the city flips to the faction.
If a city->poverty > 0.3 -> Depete storages
    city->poverty > 0.5 -> Buildings destroyed randomly
    city->poverty > 0.8 in 2 cities within a radius of 10 -> create a new faction.
```

### Prosperity

Three type of population on each city

Indigent → Content → Wealthy

Luxury points 'heat' the city 'prosperity' bar which has two values, the 'Subsistant' and 'Affluence'.  Subsistant is always lower than affluence.   These thresholds determine how many pop elements belong to each class.

If indigent is higher, cities can revolt (create a new faction) or they can flip to a different faction.


**Remaining Questions** 

* How to avoid settling a lot of cities, penalizing a high number of cities >> I can use culture to merge two cities into one.
* How to handle population growth, happiness, culture, population education >> Luxuries will represent pop economic development
* How to handle Religion and slaves >> Slaves are free labor that can join cities without consuming food.
* How to handle poverty, wealth and government style >> Indigent - Content - Wealthy
* This will allow to make sense to buy luxury products (I have defined many) >> Luxury products on tiles will now produce luxury resources (and/or culture).
* Culture spread alliance on the city which depends on the land owenership of the tile where the city is located.   So cities can revolt very easily.  
* The system also applies to city ownership of the tiles.  If a city is a cultural powerhouse  it can put all the tiles on its side and eventually it will absorb the neighboring city when the tile where the city is located flips. 

**Tribalism**
* Slavery allowed
* Can break treaties
* Treasury from the capital city only
* Cities are more independence in terms of their own money
* Religion: all allowed


**Despotism**
* Slavery: allowed
* Diplomacy: can break treaties
* Economic: treasure shared across cities.

**Monarchy**
* Slavery: allowed
* Diplomacy: can break treaties
* Economic: treasure shared across cities.

**Republic**
* Slavery: allowed
* Diplomacy: Congress decides.
* Economic: cities handle their own money

**Democracy**
* Slavery: not allowed
* Diplomacy: Congress decides.
* Economic: cities handle their own money
* Social: cities revolt easily.

**Theocracy**
* Slavery: allowed for other religion.  If a city has 100% the state religion, slaves are converted.
* Diplomacy: Congress decides.
* Economic: cities handle their own money
* Social: cities with the same religion cannot revolt.  Content is higher.

**Communism**
* Slavery: not allowed.
* Diplomacy: 
* Economic: treasure is centralized in the capital city.  The rest of the cities share what they have.
* Social: cities do not revolt at all.

**Liberal** 
* Slavery: not allowed.  All slaves are converted into citizens.
* Diplomacy: depends on city prosperity.
* Economic: cities handle their own money
* Social: cities can revolt super easily.  Luxuries are super important.

**Fascism**
* Slavery: allowed.  
* Diplomacy: no restriction
* Economic: treasure is centrlaized in the capital city.  No money on cities.
* Social: cities can revolt super easily.  Luxuries are super important.

**Renminism**
* Slavery: not allowed.  
* Diplomacy: no restriction
* Economic: treasure is centrlaized in the capital city.  No money on cities.
* Social: cities super hard to revolt.

**Technofeudalism** 
* Slavery: not allowed.  
* Diplomacy: no restriction
* Economic: treasure is centrlaized in the capital city.  No money on cities.
* Social: no wealthy people on cities, only on capital.  Tiles work depend on the amount of money, not on the population.


So, each government type cleans all the perks that established in terms of global, faction, city and set their new perks.  So the perks in the end force running paths in the code that alter the behaviour of the game.  By doing this I can have a lot of flexibility in terms of what is happening with each government type.


## Combat

Experience is important ;).  Units can get experience by combat.  Terrain plays a very important role.  Units have attack,defense, terrain, city and fortification weights.  

Terrain and city have additional weights that depend on the terrain itself and on the city (buildings).  Each unit have headcounts or soldiers that determine their size.  Each battle, soldiers inevitable die according to the stochastic power balance (on the winning side). Loosers die. So the unit is weakened in discrete steps.  A Roman Legion had 6k soldiers. Other units have also a number that represent how many soldiers the unit have. The unit needs to gather population from somewhere to increase their numbers back.   So units have a hc number (and this make sense with workers, settlers and sklaves).

- Similar to Dupuy's QJM (product of multipliers) and Sub-linear Lanchester/Helmbold (not power law, not linear on number of soldiers).
- There should be some very small stochasticity in the outcome.
- Winner wins and live and execute whatever it needs to execute (capture a city, moving into a tile, etc[).  Looser dies.
- After a battle the 'hc' of the winner is reduced some amount (outcome of the model).
- After a battle, the 'xp' of the winner increases.
- If the win was tight like 6.4 vs 6.2 then the amount of 'xp' increases more.
- Terrain affects the outcome. For instance, defending from high ground (mountains, hills) gives a boots to the defender.
- e.g., a 'Horseman' should receive a bonus while attacking on plains.
- e.g. a 'Horseman' should be penalized when defending a city (not attacking).
- It should be impossible for a 'Phalanx' beat a 'Tank'.  There should be a way to map that in the numbers without having to force it.  Only by using tuned constant numbers.
- A 'Legion' has legion.hc = 6000, a 'Marine' regiment, marine.hc = 10000.
- A unit will recover their original headcount (will try to) by 'Fortify'.  It will need LoS access to a city with available population (or to be inside the city itself where the recovery should be faster, depending on the distance to the city).
- A defending unit can be fortified on the defending tile.  This gives a boost in defense and also helps the unit recover hc.
- A city can have structures like a 'Walls' and 'Fortress' that increase their defense values.

So, the model looks like this:

**Unit Variables**
- Headcount (hc \in [300,10000]) clamped at 300.
- Experience (xp \in [0,100]) 
- Morale (m \in [0,1])
- Fortification level ff \in [0,1]

Headcount is determined from the amount of soldiers that the unit has.  It goes up on Fortification, goes down on battles.
Experience comes from fighting.  It always goes up.  Help to create good units. Units created in cities with barracks get an initial boost.
Morale will be determined from the faction culture-temperature on each tile.
Fortifiation comes from a unit Fortifying.  Defensive units have a bigger weighting factor and some units has zero.

**Unit Weights (Fixed per Unit type)**
- (aw, dw): attack weight and defense weight.  Positive integers, NOT capped: 'Warrior' 1,
  'Phalanx' 2, 'Chariot' 4, 'Legion' 5, 'Musketeer' 10, ... mechanized infantry 100 or whatever the
  era needs.  The ladder keeps climbing, roughly doubling per era, and that is what makes a
  Phalanx unlikely to beat a Tank: no special case, just the constants.
- tw[role][bioma]: (-1,1] terrain penalization/reward
- cw[role]: (-1,1]  city weight for the unit
- fw (-1,1] fortification weight for the unit.  Horseman for instance is zero.

**City variables**
- InCity: inCity ∈ {0,1} : the defense is on a city. 1 if defending from a city, 0 otherwise.
- (cf): city factor: calculated from the presence of defensive buildings. Between [0,1] depending on city buildings (For instance 'Fortress' has a value of 0.90)


**Terrain variables**
- terrain: calculated from (bioma & 0xf0) from the defending unit's tile.

**Stochastic variables**
- Ra = random between [0.97;1.03]
- Rb = random between [0.97;1.03]


**Combat outcome calculation**
So when a combat arises, each unit calculates:

Sa = (hc)^(0.65) (1 + 0.01 xp) (0.5 + m) aw (1 + tw[ATTACK][terrain]) (1 + cw[ATTACK] inCity) Ra
Sd = (hc)^(0.65) (1 + 0.01 xp) (0.5 + m) dw (1 + tw[DEFEND][terrain]) (1 + cw[DEFEND] inCity) (1 + inCity cf) (1+ fw ff) Rb

with each unit using its own hc, xp, m, ff and its own weights.

And,

 win = arg max (Sa, Sd)         -- on an exact tie the DEFENDER holds

 D = | Sa - Sd| / (Sa + Sd + e)

 e = 0.0001

So, a tie victory pushes D close to zero, and a crushing victory moves it towards 1.

>> Looser dies and is deleted.
>> Winner: winning unit

Finally, winner variables are updated:


 winner.hc -= winner.hc * (0.03 + 0.2 * (1 - D))
 winner.xp += 40 * (0.03 + 0.2 * (1 - D)).  (force xp ∈ [0,100])


# Working issues

* Iterate on improving the AI: focus more on production on cities and build defensive units.
* Units can be stacked together and named.
* Allow ships to be shown last:  perhaps it is better to add a number, like a Z-value that will help to determine what is shown first and what is shown last.
* Add a world edit testcase to create customized maps.

# Feature

* Market: duplicates the amount of TRADE
* Spheroid: Arriving to the south and north pole (0,-24) (0,23).
* Allow to circumvent the world using the spheroid and the relative position of the center.
* Ability to name landmarks, rivers, mountains and continents.
* Buy and sell resources from ports (like in Colonization). 
* Allow to form alliances with other factions and independence to arise from colonies.
* Deal with the problem of founding cities everywhere on the map: when cities are very close to each other, the one that produces more culture absorbs the other city, and one of them is destroyed, or refounded in the middle.
* Increase traderoutes: cities that create wagons or ships if they end up selling or buying something from other cities, automatically trade routes are established, that gives to the building (or supporting city) more trade resources.  This eliminates caravans.
* Population now represent head count, humans, people.  
* Each city has an exponential growth model that allows the **population** to increase based on current population and the availability of food.  * The **pop** value on each city is that value quantified.  Food existence allows population to growth. 
* Units also have **soldiers**.  
* Settlers are moving population, and the city that is built starts with the number of citizens.  
* Same for workers, their strength is derived from the size of the group.
* **Units**: members determine a stepped factor for the fighting equation.  The shape of this function depends on the unit.
* **Units**: the **personnel** factor is stepped.
* Hence, in battles, people die.  The stochastic factor determines how many casualties the winning unit lost.
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
* https://codeberg.org/rhorvat/OpenCivOne
