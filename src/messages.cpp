#include <iostream>
#include <cstdio>
#include <cstdarg>

#include "messages.h"

extern std::vector<Message> messages;

void message(int year, int faction, const char *szFormat, ...)
{
    char msg[256];

    va_list args;
    va_start(args, szFormat);
    vsprintf(msg,szFormat, args);               // @NOTE Varadic string formatting !
    va_end(args);

    Message mg;
    mg.faction = faction;// @FIXME:  Allow the posibility of adding two factions particularly when messages concern two warring factions.
    mg.msg = std::string(msg); mg.year = year;
    messages.insert(messages.begin(), mg);     
}


