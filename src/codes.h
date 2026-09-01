#ifndef CODES_H
#define CODES_H

// Dependency codes (codeId) for the Dependency Evaluation Engine (dee.h). Registered against
// a context -- see dee.h's worldContext()/factionContext()/cityContext() -- via
// DependencyEvaluationEngine::regDep(contextId, codeId).

// Tech dependency codes (Faction level)
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



// Dependency Trees (City Level)
#define VETERAN_CODE            0x01      // Perk code for veteran units 
#define HALF_POPULATION_CODE    0x02     // Perk code for half population growth
#define STORAGE_EXPANSION_1     0x0a    // Perk code for storage expansion level 1
#define STORAGE_EXPANSION_2     0x0b    // Perk code for storage expansion level 2

#endif // CODES_H
