#include <unordered_map>
#include <string>

#include "openglutils.h"
#include "font/DrawFonts.h"
#include "City.h"
#include "resources.h"
#include "coordinator.h"
#include "infoui.h"

extern std::unordered_map<int, City*> cities;
extern Coordinator coordinator;

// Icon paths for the core resources, populated once by initCoreResources() (cityscreenui.cpp).
extern std::unordered_map<int, std::string> coreresources;

// Tiles the info screen's border with the info{top,bottom,left,right,topleft,topright,
// bottomleft,bottomright}.png set, following the same corner/edge layout as
// cityscreenui.cpp's drawBoundingBox -- here in HUD screen space (placeMark, centered,
// 20px tiles) instead of map space (place, 16px tiles).
static void drawInfoBorder(int startcol, int startrow, int endcol, int endrow)
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

std::vector<City*> getCitiesForFaction(int faction_id)
{
    std::vector<City*> result;
    for(auto& [id, city] : cities)
        if (city->faction == faction_id)
            result.push_back(city);
    return result;
}

void drawInfoScreen()
{
    // Same HUD-space ortho/rotate setup as drawHUD()/drawIntro() (map.cpp): a screen-space
    // overlay independent of the map's own transform.
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

    drawInfoBorder(startcol, startrow, endcol, endrow);

    char str[256];
    sprintf(str, "Information");
    drawString(60,-70,1,str,0.3f,0.1f,0.1f,0.1f);

    int col_name = 60;
    int col_res[6] = {380, 500, 620, 740, 860, 980};

    int y = -160;
    for(int i=0;i<6;i++)
        placeMark(col_res[i], y, 20,20, coreresources[ALL_CORE_RESOURCES[i]].c_str());

    y -= 40;

    for(City* city : getCitiesForFaction(coordinator.v_f_id))
    {
        drawString(col_name, y, 1, city->name, 0.15f,0.1f,0.1f,0.1f);

        for(int i=0;i<6;i++)
        {
            sprintf(str, "%d", city->getProductionRate(ALL_CORE_RESOURCES[i]));
            drawString(col_res[i]-10, y, 1, str, 0.15f,0.1f,0.1f,0.1f);
        }

        y -= 30;
    }

    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}
