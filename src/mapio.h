#ifndef MAPIO_H
#define MAPIO_H

// Map persistence (saved_map.dat), split out of gamekernel.cpp into its own
// translation unit so it can be linked into the testcase and simulate builds too
// (gamekernel.cpp itself is not part of either -- see Makefile SCS/SSM).
void saveMap();
void loadMap();

#endif // MAPIO_H
