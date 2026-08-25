#ifndef CODES_H
#define CODES_H

// Dependency codes (codeId) for the Dependency Evaluation Engine (dee.h). Registered against
// a context -- see dee.h's worldContext()/factionContext()/cityContext() -- via
// DependencyEvaluationEngine::regDep(contextId, codeId).

// Tech dependency codes (README.md "Science" section, the Tech/Dep Code table). These are
// what each Buildable*Factory constructor registers via BuildableFactory::addDependencyCode()
// (buildable.h) to gate itself out of City::buildable until the tech is verified (dee.verifyDepAll
// in engine.cpp's populateCityBuildables()) -- the actual science/tech mechanism that calls
// dee.regDep() with these codes doesn't exist yet (README.md: "We will add later the science
// and tech mechanism that will register those dependencies").
#define TECH_THE_WHEEL         0x01
#define TECH_ARCHERY           0x02
#define TECH_WARRIOR_CODE      0x03
#define TECH_BRONZE_WORKING    0x04
#define TECH_IRON_WORKING      0x05
#define TECH_HORSEBACK_RIDING  0x06
#define TECH_ANIMAL_HUSBANDRY  0x07
#define TECH_POTTERY           0x08
#define TECH_CURRENCY          0x09
#define TECH_ALPHABET          0x0a
#define TECH_MINING            0x0b
#define TECH_CEREMONIAL_BURIAL 0x0c
#define TECH_WRITING           0x0d
#define TECH_MAP_MAKING        0x0e
// Not codeable from the README table as it stands: the Units/Resources tables' per-unit
// RESOURCE requirements (Iron/Copper, Horses, Elephants, etc.) have no entry in the Tech/Dep
// Code table, so no Factory below can require them yet. See BACKLOG.

#define VETERAN_CODE            0x01      // Perk code for veteran units 
#define HALF_POPULATION_CODE    0x02     // Perk code for half population growth

#endif // CODES_H
