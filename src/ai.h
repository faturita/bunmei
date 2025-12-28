#ifndef AI_H
#define AI_H

#include "units/Unit.h"

coordinate goTo(Unit* unit, bool &ok);
coordinate goTo(Unit* unit, bool &ok, int targetlat, int targetlon);
coordinate reachableHorizon(Unit* unit, int jumpingdistance, int f_id, bool &ok);

void autoPlayer();

#endif // AI_H