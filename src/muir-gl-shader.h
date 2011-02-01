#ifndef MUIR_GL_SHADER_H
#define MUIR_GL_SHADER_H
//
// C++ Declaration: muir-gl-shader
//
// Description:  OpenGL Shader initialization.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//
//

#ifdef __APPLE__
 #include <glut.h>
#else
 #include <GL/glut.h>
#endif

void muir_opengl_shader_init();
void muir_opengl_shader_switch(GLhandleARB num);
void muir_GPU_send_variables();



extern float shader_data_min;
extern float shader_data_max;
extern GLhandleARB shaderProgram;
#endif //MUIR_GL_SHADER_H
