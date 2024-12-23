#include "openglutils.h"
#include "math/yamathutil.h"
#include "lodepng.h"
#include "font/FontsBitmap.h"
#include "font/DrawFonts.h"
#include "map.h"
#include "ui.h"


void drawCityScreen(int centerlatitude, int centerlongitude)
{
    for(int lats=-10;lats<10;lats++)
        for (int lons=-10;lons<10;lons++)
        {
            if ( (lats<-3 || lats>3) || (lons<-3 || lons>3) )
            {
                int la= centerlatitude + lats;
                int lo = centerlongitude + lons;


                placeTile(lo,la,"assets/assets/general/citytexture.png");

            }

        }

    char str[256];
    sprintf (str, "Kattegat");
    int lat, lon;
    lat = centerlatitude + (+9);
    lon = centerlongitude + (-9);
    drawString(600+16*lon, 0+16*lat,0,str,0.07f);

}