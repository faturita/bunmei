#include <sstream>
#include <algorithm>

#include "openglutils.h"
#include "font/DrawFonts.h"
#include "map.h"
#include "dialog.h"

#ifdef __linux
#include <GL/glut.h>
#elif __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#endif

extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;

// Defined in hud.cpp: draws a size x size textured quad in HUD screen space (same texture
// cache as map.cpp's placeMark).  Reused here so the dialog box tiles like the rest of the HUD.
extern void placeMark4(float x, float y, int size, const char* modelName);

static const float PADDING = 16.0f;
static const float LINE_HEIGHT = 25.0f;
static const float STROKE_SCALE = 0.1f;      // same size as the teletype/message board text
static const float MAX_TEXT_WIDTH = 700.0f;
static const int TILE = 16;

static float measureWidth(const std::string &s, float scale)
{
    return (float)glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char*)s.c_str()) * scale;
}

// Greedy word-wrap: keeps adding words to the current line while it still fits maxwidth.
static std::vector<std::string> wrapText(const std::string &text, float maxwidth, float scale)
{
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string word, line;

    while (iss >> word)
    {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && measureWidth(candidate, scale) > maxwidth)
        {
            lines.push_back(line);
            line = word;
        }
        else
        {
            line = candidate;
        }
    }
    if (!line.empty())
        lines.push_back(line);
    if (lines.empty())
        lines.push_back("");

    return lines;
}

// Box geometry, shared by drawDialog and clickOnDialog so hit-testing always matches what was
// drawn.  Coordinates are "natural" screen space: (0,0) top-left, X right, Y DOWN -- the HUD's
// glRotatef(180,Z)+glRotatef(180,Y) pair (drawHUD) flips this to what drawString/placeMark4
// actually expect, so every draw call below negates Y itself.
struct DialogLayout
{
    std::vector<std::string> lines;
    float boxX, boxTop, boxWidth, boxHeight;
};

static DialogLayout computeLayout(querydialog &query)
{
    DialogLayout L;
    L.lines = wrapText(query.message, MAX_TEXT_WIDTH, STROKE_SCALE);

    float contentWidth = 0;
    for (auto &l : L.lines)
        contentWidth = std::max(contentWidth, measureWidth(l, STROKE_SCALE));
    for (auto &o : query.options)
        contentWidth = std::max(contentWidth, measureWidth(o, STROKE_SCALE));

    float rawWidth  = contentWidth + 2*PADDING;
    float rawHeight = 2*PADDING + LINE_HEIGHT*(L.lines.size() + query.options.size());

    // Snap to whole tiles so the tiled background/border lines up cleanly.
    int cols = (int)ceil(rawWidth  / TILE);
    int rows = (int)ceil(rawHeight / TILE);
    L.boxWidth  = cols * TILE;
    L.boxHeight = rows * TILE;

    L.boxX   = (SCREEN_WIDTH  - L.boxWidth ) / 2.0f;
    L.boxTop = (SCREEN_HEIGHT - L.boxHeight) / 2.0f;

    return L;
}

// Tile modelName over [x0,x1) x [y0,y1) (natural top-down space), TILE-sized quads.
static void tileArea(float x0, float y0, float x1, float y1, const char* modelName)
{
    for (float y=y0; y<y1; y+=TILE)
        for (float x=x0; x<x1; x+=TILE)
            placeMark4(x+TILE/2.0f, -(y+TILE/2.0f), TILE, modelName);
}

// A plain black outline, matching images/questiondialog.png -- NOT the city UI's blue
// tile-based border (topleft.png etc.): reusing those here read as a stray blue ribbon
// rather than a frame, since this box's own background is grey/white, not the city screen's.
static void drawDialogBorder(DialogLayout &L)
{
    float x0 = L.boxX, y0 = L.boxTop, x1 = L.boxX+L.boxWidth, y1 = L.boxTop+L.boxHeight;
    const float THICKNESS = 3.0f;

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
        glVertex3f(x0, -y0, 0);                glVertex3f(x1, -y0, 0);
        glVertex3f(x1, -(y0+THICKNESS), 0);    glVertex3f(x0, -(y0+THICKNESS), 0);

        glVertex3f(x0, -(y1-THICKNESS), 0);    glVertex3f(x1, -(y1-THICKNESS), 0);
        glVertex3f(x1, -y1, 0);                glVertex3f(x0, -y1, 0);

        glVertex3f(x0, -y0, 0);                glVertex3f(x0+THICKNESS, -y0, 0);
        glVertex3f(x0+THICKNESS, -y1, 0);      glVertex3f(x0, -y1, 0);

        glVertex3f(x1-THICKNESS, -y0, 0);      glVertex3f(x1, -y0, 0);
        glVertex3f(x1, -y1, 0);                glVertex3f(x1-THICKNESS, -y1, 0);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
}

void drawDialog(querydialog &query)
{
    if (!query.active)
        return;

    DialogLayout L = computeLayout(query);

    // texture_1 has an alpha channel: drawHUD does not itself enable blending, so without
    // this the tiled background would draw as opaque garbage/nothing.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // The whole box in one pass, tiled from boxTop so every row lands on the TILE grid --
    // splitting this at optionsTop (not itself a multiple of TILE) used to leave the last row
    // of the second tileArea() call overshooting past the box's actual bottom edge.
    tileArea(L.boxX, L.boxTop, L.boxX+L.boxWidth, L.boxTop+L.boxHeight, "assets/assets/general/texture_1.png");

    glDisable(GL_BLEND);

    drawDialogBorder(L);

    float textX = L.boxX + PADDING;
    float y = L.boxTop + PADDING + LINE_HEIGHT - 3;   // -3: nudge onto the tile's visual center.

    for (auto &line : L.lines)
    {
        drawString(textX, -y, 1, (char*)line.c_str(), STROKE_SCALE, 1.0f, 1.0f, 1.0f);
        y += LINE_HEIGHT;
    }

    for (auto &option : query.options)
    {
        drawString(textX, -y, 1, (char*)option.c_str(), STROKE_SCALE, 0.0f, 0.0f, 0.0f);
        y += LINE_HEIGHT;
    }
}

void clickOnDialog(querydialog &query, int x, int y)
{
    if (!query.active)
        return;

    DialogLayout L = computeLayout(query);

    float fx = x / (REAL_SCREEN_WIDTH*1.0f)  * SCREEN_WIDTH;
    float fy = y / (REAL_SCREEN_HEIGHT*1.0f) * SCREEN_HEIGHT;

    if (fx < L.boxX || fx > L.boxX+L.boxWidth || fy < L.boxTop || fy > L.boxTop+L.boxHeight)
        return;

    float optionsTop = L.boxTop + PADDING + LINE_HEIGHT*L.lines.size();

    if (fy < optionsTop)
        return;

    int index = (int)((fy - optionsTop) / LINE_HEIGHT);
    if (index < 0 || index >= (int)query.options.size())
        return;

    query.active = false;
    if (query.selected)
        query.selected(index);
}
