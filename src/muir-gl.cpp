
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-gl-shader.h"

#include <iostream>
#include <cmath>
#include <complex>
#include <stdlib.h>
#include <GL/glut.h>

void renderScene(void);
void changeSize(int w, int h);
void processNormalKeys(unsigned char key, int x, int y);
void processSpecialKeys(int key, int x, int y);

void processMouse(int button, int state, int x, int y);
void processMouseActiveMotion(int x, int y);
void processMousePassiveMotion(int x, int y);
void processMouseEntry(int state);

GLuint LoadTextureHD5(const std::string &filename, const unsigned int set );

// all variables initialized to 1.0, meaning 
// the triangle will initially be white
float red=1.0, blue=1.0, green=1.0;
//float angle=0.0;
float x_loc = 0.0;
float y_loc = 0.0;
float scale=1.0;
int mouse_x = 0;
int mouse_y = 0;
GLuint muirtex;

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(10,10);
    glutInitWindowSize(1000,800);
    glutCreateWindow("Muir Data Viewer");
    glutDisplayFunc(renderScene);
    glutIdleFunc(renderScene);

    glutReshapeFunc(changeSize);

    //Input
    glutKeyboardFunc(processNormalKeys);
    glutSpecialFunc(processSpecialKeys);	

    //adding here the mouse processing callbacks
    glutMouseFunc(processMouse);
    glutMotionFunc(processMouseActiveMotion);
    glutPassiveMotionFunc(processMousePassiveMotion);
    glutEntryFunc(processMouseEntry);

    muirtex = LoadTextureHD5("/scratch/bellamy/d0000598-decoded.h5", 1 );

    // Enable Depth Test
    glEnable(GL_DEPTH_TEST);
    glEnable( GL_TEXTURE_2D );

    // Check texture size
    GLint texSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
    std::cout << "Maximum texture size: " << texSize << std::endl;

    // Fragment shader
    muir_opengl_shader();
    
    glutMainLoop();

    return 0;
}


void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    //glRotatef(angle,0.0,1.0,0.0);

    glScalef(scale,scale,1);
    glTranslatef(x_loc/100,y_loc/100,0);
    // this is where we set the actual color
    // glColor specifies the color of all further drawings
    //glColor3f(red,green,blue);

    const float texboundx = 500.0/512.0;
    const float texboundy = 1100.0/2048.0;

    for (float i = 0; i < 100; i += 0.50)
    {
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, muirtex );
        glBegin( GL_QUADS );
        glTexCoord2d(0.0,0.0); glVertex2d(i,0.0);
        glTexCoord2d(texboundx,0.0); glVertex2d(i+ 0.25,0.0);
        glTexCoord2d(texboundx,texboundy); glVertex2d(i+ 0.25,1.0);
        glTexCoord2d(0.0,texboundy); glVertex2d(i,1.0);
        glEnd();
    }

    glDisable( GL_TEXTURE_2D );
    glColor3f(1.0f,1.0f,1.0f);
    glBegin( GL_QUADS );
    glVertex2d(0.5,0.5);
    glVertex2d(1.0,0.5);
    glVertex2d(1.0,1.0);
    glVertex2d(0.5,1.0);
    glEnd();
    glPopMatrix();

    glutSwapBuffers();

}


void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
    if(h == 0)
        h = 1;

    float ratio = 1.0* w / h;

	// Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the correct perspective.
    gluPerspective(45,ratio,1,1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0,0.0,5.0, 
              0.0,0.0,-1.0,
              0.0f,1.0f,0.0f);


}

void processNormalKeys(unsigned char key, int x, int y) {

    if (key == 27) 
        exit(0);
    else if (key=='r') {
        int mod = glutGetModifiers();
        if (mod == GLUT_ACTIVE_ALT)
            red = 0.0;
        else
            red = 1.0;
    }
}

void processSpecialKeys(int key, int x, int y) {

    int mod;
    switch(key) {
        case GLUT_KEY_F1 : 
            mod = glutGetModifiers();
            if (mod == (GLUT_ACTIVE_CTRL|GLUT_ACTIVE_ALT)) {
                red = 1.0; green = 0.0; blue = 0.0;
            }
            break;
        case GLUT_KEY_F2 : 
            red = 0.0; 
            green = 1.0; 
            blue = 0.0; break;
        case GLUT_KEY_F3 : 
            red = 0.0; 
            green = 0.0; 
            blue = 1.0; break;
    }
}


void processMouse(int button, int state, int x, int y) {


//    specialKey = glutGetModifiers();
	// if both a mouse button, and the ALT key, are pressed  then
    if (state == GLUT_DOWN)
    {
        //std::cout << "Button:" << button << std::endl;
        if (button == 3) // scroll up
            scale *= 1.5f;
        
        if (button == 4) // scroll down
            scale /= 1.5f;
    }
  //       (specialKey == GLUT_ACTIVE_ALT)) {

		// set the color to pure red for the left button
       // if (button == GLUT_LEFT_BUTTON) {
       //     red = 1.0; green = 0.0; blue = 0.0;
       // }
		// set the color to pure green for the middle button
       // else if (button == GLUT_MIDDLE_BUTTON) {
       //     red = 0.0; green = 1.0; blue = 0.0;
       // }
		// set the color to pure blue for the right button
       // else {
        //    red = 0.0; green = 0.0; blue = 1.0;
        //}
    //}

}


void processMouseActiveMotion(int x, int y) {
#if 0
	// the ALT key was used in the previous function
    if (specialKey != GLUT_ACTIVE_ALT) {
		// setting red to be relative to the mouse 
		// position inside the window
        if (x < 0)
            red = 0.0;
        else if (x > width)
            red = 1.0;
        else
            red = ((float) x)/height;
		// setting green to be relative to the mouse 
		// position inside the window
        if (y < 0)
            green = 0.0;
        else if (y > width)
            green = 1.0;
        else
            green = ((float) y)/height;
		// removing the blue component.
        blue = 0.0;
    }
#endif
    if (x < mouse_x)
       x_loc -= (mouse_x-x)/scale;
    else
       x_loc += (x-mouse_x)/scale;

    mouse_x = x;

    if (y < mouse_y)
        y_loc += (mouse_y-y)/scale;
    else
        y_loc -= (y-mouse_y)/scale;

    mouse_y = y;
}


void processMousePassiveMotion(int x, int y) {

	// User must press the SHIFT key to change the 
	// rotation in the X axis
    //if (specialKey != GLUT_ACTIVE_SHIFT) {

		// setting the angle to be relative to the mouse 
		// position inside the window
        //if (x < 0)
           // angle += x;
        //else if (x > width)
            //angle = 180.0;
        //else
            //angle = 180.0 * ((float) x)/height;
    //}
   mouse_x = x;
   mouse_y = y;
}


void processMouseEntry(int state) {
    //if (state == GLUT_LEFT)
    //    deltaAngle = 0.0;/scratch/bellamy/d0000598-decoded.h5
    //else
    //    deltaAngle = 1.0;
}


GLuint LoadTextureHD5(const std::string &filename, const unsigned int set )
{
    GLuint texture;
    int width, height;
    GLfloat * data;

    // Open file
    MuirHD5 h5file( filename.c_str(), H5F_ACC_RDONLY );

    // Get data
    Muir3DArrayF decoded_data;
    h5file.read_3D_float(RTI_DECODEDDATA_PATH, decoded_data);

    // Get dataset shape
    const Muir3DArrayF::size_type *array_dims = decoded_data.shape();
    assert(decoded_data.num_dimensions() == 3);

    Muir3DArrayF::size_type dataset_count = array_dims[0];  // # sets
    Muir3DArrayF::size_type dataset_width = array_dims[1];  // frames per set
    Muir3DArrayF::size_type dataset_height = array_dims[2]; // Range bins

    // Finding maxes and mins
    std::cerr << "Determining data mins and maxes for color pallete" << std::endl;
    float data_min = log10(decoded_data[0][0][0]+1)*10;
    float data_max = log10(decoded_data[0][0][0]+1)*10;

    for (Muir3DArrayF::size_type seti = 0; seti < dataset_count; seti++)
    {
        for (Muir3DArrayF::size_type col = 0; col < dataset_width; col++)
        {
            for (Muir3DArrayF::size_type row = 0; row < dataset_height; row++)
            {
                float sample = log10(decoded_data[seti][col][row]+1)*10;
                data_min = std::min(data_min,sample);
                data_max = std::max(data_max,sample);
            }
        }
    }

    // allocate buffer
    //width = 500;
    //height = 1100;
    width = 512;
    height = 2048;
    //width  = 512;
    //height = 512;
    data = reinterpret_cast<GLfloat*>(malloc( width * height*sizeof(GLfloat) ));

    for (Muir3DArrayF::size_type col = 0; col < width ;col++)
    {
        for (Muir3DArrayF::size_type row = 0; row < height; row++)
        {
            data[row*width + col] = .50;
        }
    }
    
    for (Muir3DArrayF::size_type col = 0; col < dataset_width ;col++)
    {
        for (Muir3DArrayF::size_type row = 0; row < dataset_height; row++)
        {
                float pixel = log10(decoded_data[set][col][row]+1)*10;
                //unsigned char pixel = static_cast<unsigned char>((sample-data_min)/(data_max-data_min)*255);
                data[row*width + col] = pixel;
        }
    }

    // allocate a texture name
    glGenTextures( 1, &texture );

    // select our current texture
    glBindTexture( GL_TEXTURE_2D, texture );

    // select modulate to mix texture with color for shading
    //glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    // when texture area is small, bilinear filter the closest mipmap
    //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    //                 GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 //In this case, the driver will convert your 32 bit float to 16 bit float
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16F_ARB, width, height, 0, GL_LUMINANCE, GL_FLOAT, data);

    
    // if wrap is true, the texture wraps over at the edges (repeat)
    //       ... false, the texture ends at the edges (clamp)
    //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    // build our texture
    //glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE16, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    //gluBuild2DMipmaps( GL_TEXTURE_2D, GL_LUMINANCE16, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, data );
    //gluBuild2DMipmaps( GL_TEXTURE_2D, GL_LUMINANCE16F_ARB, width, height, GL_LUMINANCE32F_ARB, GL_FLOAT, data );

    // free buffer
    free( data );

    return texture;
}
