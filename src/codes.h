#ifndef CODES_H
#define CODES_H

// Dependency codes (codeId) for the Dependency Evaluation Engine (dee.h). Registered against
// a context -- see dee.h's worldContext()/factionContext()/cityContext() -- via
// DependencyEvaluationEngine::regDep(contextId, codeId).

// Tech dependency codes (Faction level).
//
// These ARE the technology codes of README.md's "Science" table -- one code per node of the
// tech graph, so a Tech's graph id and the dependency code it registers on discovery are the
// same number (technologies.h). Adding a technology means adding a row here AND a row in the
// graph the game builds; a code with no buildable gated behind it yet is fine.
//
// Codes are only unique WITHIN a context type: a faction-scoped TECH_* may share a number
// with a city-scoped code below, which is exactly what dee.h's context tagging is for.
#define TECH_LANGUAGE           0x01
#define TECH_HUNTING            0x02
#define TECH_AGRICULTURE        0x03
#define TECH_FISHING            0x04
#define TECH_MINING             0x05
#define TECH_MASONRY            0x06
#define TECH_THE_WHEEL          0x07
#define TECH_ARCHERY            0x08
#define TECH_WARRIOR_CODE       0x09
#define TECH_BRONZE_WORKING     0x0a
#define TECH_ANIMAL_HUSBANDRY   0x0b
#define TECH_POTTERY            0x0c
#define TECH_ALPHABET           0x0d
#define TECH_CEREMONIAL_BURIAL  0x0e
#define TECH_WRITING            0x0f
#define TECH_MATHEMATICS        0x10
#define TECH_IRON_WORKING       0x11
#define TECH_HORSEBACK_RIDING   0x12
#define TECH_CONSTRUCTION       0x13
#define TECH_CURRENCY           0x14
#define TECH_MYSTICISM          0x15
#define TECH_MAP_MAKING         0x16
#define TECH_POLYTHEISM         0x17
#define TECH_LITERATURE         0x18
#define TECH_CODE_OF_LAWS       0x19
#define TECH_PHILOSOPHY         0x1a
#define TECH_METAL_CASTING      0x1b
#define TECH_MONOTHEISM         0x1c
#define TECH_REPUBLIC           0x1d
#define TECH_MONARCHY           0x1e
#define TECH_FEUDALISM          0x1f
#define TECH_SHIP_BUILDING      0x20
#define TECH_THEOLOGY           0x21
#define TECH_EDUCATION          0x22
#define TECH_ASTRONOMY          0x23
#define TECH_BANKING            0x24
#define TECH_CHIVALRY           0x25
#define TECH_PHYSICS            0x26
#define TECH_GUNPOWDER          0x27
#define TECH_MAGNETISM          0x28
#define TECH_CHEMISTRY          0x29
#define TECH_METALLURGY         0x2a
#define TECH_CHARTERS           0x2b
#define TECH_MUSIC              0x2c
#define TECH_INDUSTRIALIZATION  0x2d
#define TECH_MILITARY_TRADITION 0x2e

// Lowest and highest technology code, so a range check does not have to hardcode the ends.
#define TECH_FIRST              TECH_LANGUAGE
#define TECH_LAST               TECH_MILITARY_TRADITION
#define TECH_COUNT              (TECH_LAST - TECH_FIRST + 1)

// The root every faction starts knowing (README.md: "Language" has no dependencies).
#define TECH_ROOT               TECH_LANGUAGE


// Dependency Trees (City Level)
#define VETERAN_CODE            0x01      // Perk code for veteran units
#define HALF_POPULATION_CODE    0x02     // Perk code for half population growth
#define STORAGE_EXPANSION_1     0x0a    // Perk code for storage expansion level 1
#define STORAGE_EXPANSION_2     0x0b    // Perk code for storage expansion level 2

#endif // CODES_H
