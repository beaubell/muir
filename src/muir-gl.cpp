
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-gl-shader.h"
#include "muir-gl-data.h"

#include <iostream>
#include <cmath>
#include <complex>
#include <stdlib.h>
#include <GL/glut.h>
#include <vector>

void renderScene(void);
void changeSize(int w, int h);
void processNormalKeys(unsigned char key, int x, int y);
void processSpecialKeys(int key, int x, int y);

void processMouse(int button, int state, int x, int y);
void processMouseActiveMotion(int x, int y);
void processMousePassiveMotion(int x, int y);
void processMouseEntry(int state);

void LoadTextureHD5(const std::string &filename, std::vector<Muirgl_Data> &data );

// all variables initialized to 1.0, meaning 
// the triangle will initially be white
float red=1.0, blue=1.0, green=1.0;
//float angle=0.0;
float x_loc = 0.0;
float y_loc = 0.0;
float scale=1.0;
int mouse_x = 0;
int mouse_y = 0;
int frame_max = 0;
int frame_min = 0;

std::vector<Muirgl_Data> data;

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

    /// FIXME, load data from file
    //LoadTextureHD5("/scratch/bellamy/d0000648-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000601-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000602-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000603-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000604-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000605-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000606-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000607-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000608-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000609-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000610-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000611-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000612-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000613-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000614-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000615-decoded.h5", data);
    LoadTextureHD5("/scratch/bellamy/d0000616-decoded.h5", data);

    // Find framecounts min/max
    frame_max = data[0].frameend;
    frame_min = data[0].framestart;
    for(std::vector<Muirgl_Data>::iterator iter = data.begin(); iter != data.end(); iter++)
    {
        if(iter->frameend > frame_max)
            frame_max = iter->frameend;
        if(iter->framestart > frame_min)
            frame_min = iter->framestart;
    }

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

    // FIXME, use more parameters from the data.
    const float texboundx = 500.0/512.0;
    const float texboundy = 1100.0/2048.0;

    for(std::vector<Muirgl_Data>::iterator iter = data.begin(); iter != data.end(); iter++)
    {
        float x = static_cast<float>((frame_min - iter->framestart))/2000.0;
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, iter->texnum );
        glBegin( GL_QUADS );
        glTexCoord2d(0.0,0.0); glVertex2d(x,0.0);
        glTexCoord2d(texboundx,0.0); glVertex2d(x+ 0.25,0.0);
        glTexCoord2d(texboundx,texboundy); glVertex2d(x+ 0.25,1.0);
        glTexCoord2d(0.0,texboundy); glVertex2d(x,1.0);
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
    if (key == 'q')
    {
        shader_data_min += 0.5f;
        GPU_send_variables();
    }
    if (key == 'a')
    {
        shader_data_min -= 0.5f;
        GPU_send_variables();
    }
    if (key == 'w')
    {
        shader_data_max += 0.5f;
        GPU_send_variables();
    }
    if (key == 's')
    {
        shader_data_max -= 0.5f;
        GPU_send_variables();
    }
    std::cout << "Min: " << shader_data_min << ", Max: " << shader_data_max << std::endl;
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


void LoadTextureHD5(const std::string &filename, std::vector<Muirgl_Data> &datavec )
{
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

    // Get framecount data
    Muir2DArrayUI framecount;
    h5file.read_2D_uint(RTI_DECODEDFRAME_PATH, framecount);

    // texture width
    width = 512;
    height = 2048;

    for (unsigned int set = 0; set < dataset_count; set++)
    {
        Muirgl_Data dataptr;
        dataptr.texwidth = width;
        dataptr.texheight = height;
        dataptr.datawoffset = 0;
        dataptr.datahoffset = 0;
        dataptr.dataw = dataset_width;
        dataptr.datah = dataset_height;
        dataptr.framestart = framecount[set][0];
        dataptr.frameend = framecount[set][dataset_width-1];

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
                    data[row*width + col] = pixel;
            }
        }

        // allocate a texture name
        glGenTextures( 1, &dataptr.texnum );

        // select our current texture
        glBindTexture( GL_TEXTURE_2D, dataptr.texnum );

        // when texture area is large, bilinear filter the first mipmap
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //In this case, the driver will convert your 32 bit float to 16 bit float
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16F_ARB, width, height, 0, GL_LUMINANCE, GL_FLOAT, data);

        // free buffer
        free( data );

        // Add GPU texture data to vector
        datavec.push_back(dataptr);

        // Geewhiz stuff
#if 0
        #define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
        #define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

        GLint total_mem_kb = 0;
                glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX,
                            &total_mem_kb);
        
        GLint cur_avail_mem_kb = 0;
                glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX,
                            &cur_avail_mem_kb);
#endif
        std::cout << "Loaded set: " << filename << ":" << set << "   texnum:" << dataptr.texnum << 
                 " Glerror?: " << glGetError() << std::endl;
    }
    //return texture;
}
