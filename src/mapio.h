#ifndef MAPIO_H
#define MAPIO_H

#include <string>

// Map persistence (saved_map.dat), split out of gamekernel.cpp into its own
// translation unit so it can be linked into the testcase and simulate builds too
// (gamekernel.cpp itself is not part of either -- see Makefile SCS/SSM).
void saveMap(const std::string &path = "saved_map.dat");
void loadMap(const std::string &path = "saved_map.dat");

#endif // MAPIO_H
