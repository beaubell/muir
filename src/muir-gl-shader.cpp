//
// C++ Implementation: muir-gl-shader
//
// Description: OpenGL Shader initialization.
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
//
//
//
#define GL_GLEXT_PROTOTYPES 1

#include "muir-gl-shader.h"
#include "muir-utility.h"

#ifdef __APPLE__
 #include <glut.h>
 #include <OpenGL/glext.h>
#else
 #include <GL/glut.h>
 #include <GL/glext.h>
#endif

#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdio>

GLhandleARB shaderProgram;
int shader_data_min_loc;
int shader_data_max_loc;
float shader_data_min = 0.0f;
float shader_data_max = 100.0f;

std::string fragshader_source[1];

void printInfoLog(GLhandleARB obj);

// This function loads both a texture and fragment shader. (fragment parts commented out)
// Ripped from my space sim (Beau V.C. Bellamy)
void muir_opengl_shader_init()
{

    //std::string   vpath = Global.dir_shaders + vname;
    //std::string   fpath = Global.dir_shaders + fname;

    //GLhandleARB myVertexShader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    GLhandleARB myFragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    //char *vtxt = textFileRead(vpath.c_str());
    load_file("colorizer.frag", fragshader_source[0]);

    //glShaderSourceARB(myVertexShader, 1, const_cast<const char**>(&vtxt), NULL);
    const char* chararr= fragshader_source[0].c_str();
    glShaderSourceARB(myFragmentShader, 1, (const char **)&chararr, NULL);

    //glCompileShaderARB(myVertexShader);

    GLint success;
    //glGetObjectParameterivARB(myVertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &success);
    //if (!success)
    //{
    //    GLcharARB infoLog[1000];
    //    glGetInfoLogARB(myVertexShader, 1000, NULL, infoLog);
    //    std::cout << "Error in vertex shader compilation!" << std::endl;
    //    std::cout << "Info Log: " << infoLog << std::endl;
    //    return;
    // }

    success = 0;
    glCompileShaderARB(myFragmentShader);
    glGetObjectParameterivARB(myFragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &success);
    if (!success)
    {
        GLcharARB infoLog[1000];
        glGetInfoLogARB(myFragmentShader, 1000, NULL, infoLog);
        std::cout << "Error in fragment shader compilation!" << std::endl;
        std::cout << "Info Log: " << infoLog << std::endl;
        return;
    }


    shaderProgram = glCreateProgramObjectARB();
    glAttachObjectARB(shaderProgram, myFragmentShader);
    //glAttachObjectARB(Global.shaderProgram, myVertexShader);

    success = 0;

    glLinkProgramARB(shaderProgram);
    glGetObjectParameterivARB(shaderProgram, GL_OBJECT_LINK_STATUS_ARB, &success);
    if (!success)
    {
        printInfoLog(shaderProgram);
    }

    success = 0;

    glValidateProgramARB(shaderProgram);
    glGetObjectParameterivARB(shaderProgram, GL_OBJECT_VALIDATE_STATUS_ARB, &success);
    if (!success)
    {
        GLcharARB infoLog[1000];
        glGetInfoLogARB(shaderProgram, 1000, NULL, infoLog);
        std::cout << "Error in program validation!" << std::endl;
        std::cout << "Info Log: " << infoLog << std::endl;
    }

    glUseProgramObjectARB(shaderProgram);
    shader_data_min_loc = glGetUniformLocationARB(shaderProgram, "data_min");
    shader_data_max_loc = glGetUniformLocationARB(shaderProgram, "data_max");
    muir_GPU_send_variables();

}

void muir_opengl_shader_switch(GLhandleARB num)
{
    glUseProgramObjectARB(num);
}

void muir_GPU_send_variables()
{
    glUniform1fARB(shader_data_min_loc, shader_data_min);
    glUniform1fARB(shader_data_max_loc, shader_data_max);
}

void printInfoLog(GLhandleARB obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB,
                              &infologLength);

    if (infologLength > 0)
    {
        infoLog = reinterpret_cast<char *>(malloc(infologLength));
        glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n",infoLog);
        free(infoLog);
    }
}

