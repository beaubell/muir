#ifndef MUIR_GL_DATA_H
#define MUIR_GL_DATA_H
//
// C++ Declaration: Muir OpenGL data rendering
//
// Description: Loads and renders muir data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//

#include "muir-data.h"
#include "muir-types.h"

#ifdef __APPLE__
 #include <glut.h>
 #include <OpenGL/glext.h>
#else
 #include <GL/glut.h>
 #include <GL/glext.h>
#endif

#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
namespace FS = boost::filesystem;

enum Data_Type { FILE_COMPLEX, FILE_DECODED };

class Muirgl_Data {
    public:
        std::vector<GLuint> texnames;
        GLuint texwidth;
        GLuint texheight;
        GLuint dataw;
        GLuint datah;
        double radacstart;
        double radacend;
        FS::path file_raw;
        FS::path file_decoded;

        Muir2DArrayF  sample_range;
        Muir2DArrayUI framecount;
        Muir2DArrayD  radac_time;

        Muirgl_Data(FS::path file);

        void render(const double radac_min,const bool texture_smooth);
        void stage();
        void release();

    private:
        unsigned int _sets;
        bool _staged;
        Data_Type type;

        void load3D(const MuirHD5& file, const std::string& path);
        void load4D(const MuirHD5& file, const std::string& path);
};

#endif //MUIR_GL_DATA_H
