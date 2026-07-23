#ifndef DIALOG_H
#define DIALOG_H

#include <string>
#include <vector>
#include <functional>

// A simple modal Yes/No-style dialog box (see images/questiondialog.png): a message and a
// list of clickable options.  Set message/options and selected, then set active=true to pop
// it up; it draws on top of whatever view is active and consumes the next click itself.
struct querydialog
{
    bool active = false;
    std::string message;
    std::vector<std::string> options;
    std::function<void(int)> selected;      // called with the chosen option's index
};

void drawDialog(querydialog &query);

// x,y are raw window coordinates, exactly as received by processMouse.
void clickOnDialog(querydialog &query, int x, int y);

#endif // DIALOG_H
