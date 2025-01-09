#ifndef AI_H
#define AI_H

#include "units/Unit.h"

coordinate goTo(Unit* unit, bool &ok);
coordinate reachableHorizon(Unit* unit, int jumpingdistance, int f_id, bool &ok);

#endif // AI_H