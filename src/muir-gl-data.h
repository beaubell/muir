#ifndef MUIR_GL_DATA_H
#define MUIR_GL_DATA_H
//
// C++ Declaration: muir OpenGL Shader initialization
//
// Description: Constants for MUIR experiment data.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//
//

#ifdef __APPLE__
 #include <glut.h>
 #include <OpenGL/glext.h>
#else
 #include <GL/glut.h>
 #include <GL/glext.h>
#endif

class Muirgl_Data {
    public:
        GLuint texnum;
        GLuint texwidth;
        GLuint texheight;
        GLuint datawoffset;
        GLuint datahoffset;
        GLuint dataw;
        GLuint datah;
        double radacstart;
        double radacend;

        Muirgl_Data();
};

#endif //MUIR_GL_DATA_H
