#ifndef MESSAGES_H
#define MESSAGES_H

#include <iostream>

struct Message
{
    int year;
    std::string msg;
    int faction;
};

void message(int year, int faction, const char *szFormat, ...);

#endif // MESSAGES_H