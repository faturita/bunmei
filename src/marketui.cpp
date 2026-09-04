#include <unordered_map>
#include <string>
#include <cstdio>

#include "openglutils.h"
#include "font/DrawFonts.h"
#include "City.h"
#include "Faction.h"
#include "resources.h"
#include "tiles.h"
#include "map.h"
#include "coordinator.h"
#include "marketui.h"

extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Coordinator coordinator;
extern Map map;
extern Tiles tiles;

// resource id -> total price of that resource (prices[], seeded to 1 by initPrices()).
extern std::unordered_map<int, int> prices;

// Same border as the info screen (infoui.cpp): the info{...}.png tile set laid out like
// cityscreenui.cpp's drawBoundingBox, in HUD screen space (placeMark, 20px tiles).
static void drawMarketBorder(int startcol, int startrow, int endcol, int endrow)
{
    placeMark(startcol*20, -startrow*20, 20,20, "assets/assets/general/infotopleft.png");
    for(int i=startcol+1;i<endcol;i++)
    {
        placeMark(i*20, -startrow*20, 20,20, "assets/assets/general/infotop.png");
        placeMark(i*20, -endrow*20, 20,20, "assets/assets/general/infobottom.png");
    }
    placeMark(endcol*20, -startrow*20, 20,20, "assets/assets/general/infotopright.png");
    placeMark(startcol*20, -endrow*20, 20,20, "assets/assets/general/infobottomleft.png");
    for(int i=startrow+1;i<endrow;i++)
    {
        placeMark(startcol*20, -i*20, 20,20, "assets/assets/general/infoleft.png");
        placeMark(endcol*20, -i*20, 20,20, "assets/assets/general/inforight.png");
    }
    placeMark(endcol*20, -endrow*20, 20,20, "assets/assets/general/infobottomright.png");
}

std::unordered_map<int,int> getShippableStockForFaction(int faction_id)
{
    std::unordered_map<int,int> total;
    for (int id : ALL_COMMODITIES) total[id] = 0;
    for (int id : ALL_MFG_GOODS)   total[id] = 0;

    for (auto& [k, city] : cities)
    {
        // Same "is this city visible to the viewing faction" test drawUnitsAndCities()
        // (map.cpp) uses: convert to screen coords, then check the cell's per-faction fog.
        coordinate co = map.to_screen(city->latitude, city->longitude);
        if (!map(co.lat, co.lon).isVisible(faction_id))
            continue;

        for (int id : ALL_COMMODITIES) total[id] += city->commodities[id];
        for (int id : ALL_MFG_GOODS)   total[id] += city->mfggoods[id];
    }
    return total;
}

void drawMarketScreen()
{
    // Same HUD-space ortho/rotate setup as drawInfoScreen()/drawHUD().
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1200, 800, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_CURRENT_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1);
    glDisable(GL_DEPTH_TEST);
    glRotatef(180.0f,0,0,1);
    glRotatef(180.0f,0,1,0);

    const int startcol = 1, endcol = 59, startrow = 1, endrow = 39;

    drawBox((startcol+endcol)*10.0f, -(startrow+endrow)*10.0f, (endcol-startcol)*20, (endrow-startrow)*20, 0.93f,0.93f,0.88f);
    drawMarketBorder(startcol, startrow, endcol, endrow);

    char str[256];
    sprintf(str, "Market");
    drawString(60,-70,1,str,0.3f,0.1f,0.1f,0.1f);
    sprintf(str, "prices and stock across %s's visible cities", factions[coordinator.v_f_id]->name);
    drawString(60,-100,1,str,0.12f,0.1f,0.1f,0.1f);

    std::unordered_map<int,int> stock = getShippableStockForFaction(coordinator.v_f_id);

    // Build the full list: commodities first, then mfg goods (same order as everywhere else).
    std::vector<int> all;
    for (int id : ALL_COMMODITIES) all.push_back(id);
    for (int id : ALL_MFG_GOODS)   all.push_back(id);

    // Two columns of rows so all ~34 fit vertically. Per column: icon, "Price", "Stock".
    const int ROWS_PER_COL = 17;
    const int colX[2][3] = { {70, 200, 320}, {620, 750, 870} };   // icon / price / stock

    for (int c=0;c<2;c++)
    {
        drawString(colX[c][1], -130, 1, (char*)"Price", 0.12f,0.1f,0.1f,0.1f);
        drawString(colX[c][2], -130, 1, (char*)"Stock", 0.12f,0.1f,0.1f,0.1f);
    }

    for (int i=0;i<(int)all.size();i++)
    {
        int id = all[i];
        int col = i / ROWS_PER_COL;
        int row = i % ROWS_PER_COL;
        int y = -160 - row*34;

        placeMark(colX[col][0], y, 20,20, tiles[id].c_str());

        int price = prices.count(id) ? prices[id] : 0;
        sprintf(str, "%d", price);
        drawString(colX[col][1], y, 1, str, 0.15f,0.1f,0.1f,0.1f);

        sprintf(str, "%d", stock[id]);
        drawString(colX[col][2], y, 1, str, 0.15f,0.1f,0.1f,0.1f);
    }

    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}
