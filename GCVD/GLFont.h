#ifndef GLFONT_H
#define GLFONT_H

#include <map>

#include <GL/gl.h>
#include <GL/glut.h>
#include "GLHelpers.h"

#include "FontStructs.h"

#include "mono.h"
#include "sans.h"
#include "serif.h"

namespace GLXInterface {

struct FontData {

    typedef std::map<std::string, Font*> FontMap;

    FontData() {
        fonts["sans"] = &sans_font;
        fonts["mono"] = &mono_font;
        fonts["serif"] = &serif_font;
        GLXInterface::glSetFont("sans");
    }
    inline Font* currentFont() { return fonts[currentFontName]; }

    std::string currentFontName;
    FontMap fonts;
};

static struct FontData data;

}  // end namespace GLXInterface

#endif