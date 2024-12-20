#include "../profiling.h"
#include "../math/yamathutil.h"
#include "../lodepng.h"
#include "../openglutils.h"

std::unordered_map<std::string, GLuint> fonts;


GLuint preloadFontTexture(const char* letterName, const char* modelName)
{
    GLuint _texture;

    if (fonts.find(std::string(letterName)) == fonts.end())
    {
        unsigned char *img;

        unsigned w,h;

        lodepng_decode_file(&img, &w, &h, modelName, LCT_RGBA, 8);

        Image image((char *)img, w, h);

        for(int i=0;i<w;i++)
            for (int j=0;j<h;j++)
            {
                if (img[(i*h+j)*4+0]<100)
                {
                    img[(i*h+j)*4+3] = 0;
                }
            }

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     w,h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     img);


        fonts[std::string(letterName)]=_texture;

    } else {
        _texture = fonts[std::string(letterName)];
    }

    return _texture;
}

void preloadFonts()
{
    for(int letter=32;letter<128;letter++)
    {
        char modelName[256];
        sprintf(modelName,"assets/assets/general/%d.png",letter);
        printf("Letter %c in %s\n",(char)letter, modelName);
        char letterName[256];
        sprintf(letterName,"%c",letter);
        preloadFontTexture(letterName, modelName);
    }

}



/**
 * The font1.png is a font containing 128 characters, 8x16 pixels each.
 * So each column contains 16 characters
 * And there are 8 rows.
 * Characters are separataed by 1-pixel.  So it has 128+17 pixels width, and 128+9 pixels height.
 */
void  preloadFont(const char* fontName)
{
    unsigned char *img;

    unsigned width,height;

    lodepng_decode_file(&img, &width, &height, fontName, LCT_RGBA, 8);

    printf("Font size w=%d,h=%d, image: %d\n", width,height, sizeof(img));
    char letter = 32;

    for(int col=0;col<16;col++)
    for(int row=2;row<8;row++)
    {
        unsigned char *font = new unsigned char[8*16*4];

        unsigned h = 16;
        unsigned w = 8;

        for(int i=0;i<w;i++)
            for(int j=0;j<h;j++)
            {

            }

        for(int i=0;i<w;i++)
            for (int j=0;j<h;j++)
            {
                {
                    //font[(i*h+j)*4+0] = 255;
                    //font[(i*h+j)*4+1] = 255;
                    //font[(i*h+j)*4+2] = 0;
                    //font[(i*h+j)*4+3] = 255;
                }
            }

        char modelName[256];
        sprintf(modelName,"%c",letter);
        GLuint _texture;
        letter++;
        if (fonts.find(std::string(modelName)) == fonts.end())
        {


            Image image((char *)font, w, h);

            glGenTextures(1, &_texture);
            glBindTexture(GL_TEXTURE_2D, _texture);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_RGBA,
                        w,h,
                        0,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        font);


            fonts[std::string(modelName)]=_texture;

            printf("Added letter %s, %d, %d into %d \n",modelName, w,h, _texture);

        } else {
            _texture = fonts[std::string(modelName)];
        }
    }
}

void placeLetter(float x, float y, const char* letter)
{
    GLuint _texture = fonts[std::string(letter)];

    placeMark(x,y,8,16,_texture);
}

void placeWord(float x, float y, const char* word)
{
    for(int i=0;i<strlen(word);i++)
    {
        char letter[2];
        letter[0] = word[i];
        letter[1] = '\0';
        placeLetter(x+i*8,y,letter);
    }
}