
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-gl-shader.h"
#include "muir-gl-data.h"

#include <iostream>
#include <cmath>
#include <complex>
#include <stdlib.h>

#ifdef __APPLE__
 #include <glut.h>
#else
 #include <GL/glut.h>
#endif

#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/timer.hpp>
#include <boost/lexical_cast.hpp>
namespace FS = boost::filesystem;
namespace T = boost::filesystem;

void renderScene(void);
void idleFunc(void);
void changeSize(int w, int h);
void processNormalKeys(unsigned char key, int x, int y);
void processSpecialKeys(int key, int x, int y);

void processMouse(int button, int state, int x, int y);
void processMouseActiveMotion(int x, int y);
void processMousePassiveMotion(int x, int y);
void processMouseEntry(int state);

void LoadHD5Meta(const std::string &filename, std::vector<Muirgl_Data> &data );
void loadfiles(const std::string &dir);

// Global state
int window_w = 0;
int window_h = 0;
double x_loc = 0.0;
double y_loc = 0.0;
double scale = 1.0;
int mouse_x = 0;
int mouse_y = 0;
float mouse_x_vel = 0.0f;
float mouse_y_vel = 0.0f;
double radac_max = 0.0;
double radac_min = 0.0;
double radac_position = 0.0;
float scroll_vel = 10.0f;
float scroll_acc = -2.0f;
bool texture_smooth = true;
double walltime = 0.0;

// Data Handler
std::vector<Muirgl_Data> data;



int main(int argc, char **argv)
{
    // Initialize timer state
    walltime = static_cast<double>(glutGet(GLUT_ELAPSED_TIME))/1000.0;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(10,10);
    glutInitWindowSize(1000,800);
    glutCreateWindow("Muir Data Viewer");
    glutDisplayFunc(renderScene);
    glutIdleFunc(idleFunc);

    glutReshapeFunc(changeSize);

    //Input
    glutKeyboardFunc(processNormalKeys);
    glutSpecialFunc(processSpecialKeys);	

    //adding here the mouse processing callbacks
    glutMouseFunc(processMouse);
    glutMotionFunc(processMouseActiveMotion);
    glutPassiveMotionFunc(processMousePassiveMotion);
    glutEntryFunc(processMouseEntry);

    // Fragment shader
    muir_opengl_shader_init();

    /// FIXME, load initialization data from file
    for (unsigned int i = 1; i < argc; i++)
        loadfiles(argv[i]);

    // Find radactime min/max
    radac_max = data[0].radacend;
    radac_min = data[0].radacstart;
    for(std::vector<Muirgl_Data>::iterator iter = data.begin(); iter != data.end(); iter++)
    {
            radac_max = std::max(iter->radacend, radac_max);
            radac_min = std::min(iter->radacstart, radac_min);
    }
    radac_position = radac_min;


    glDisable(GL_DEPTH_TEST); // Don't need depth testing
    glDisable(GL_LIGHTING);   // Dont need lighting

    // Check texture size
    GLint texSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
    std::cout << "Maximum texture size: " << texSize << std::endl;

    glutMainLoop();

    GLenum err = glGetError();
    if(err)
      std::cout << "OpenGL ERROR: " << err << std::endl;

    return 0;
}


void renderScene(void) {

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    
    glScaled(scale, scale, 1.0);
    glTranslated(x_loc, y_loc, 0.0);

    // FIXME, use more parameters from the data.
    //const float texboundx = 500.0f; // 500.0/512.0;
    //const float texboundy = 1100.0f; // 1100.0/2048.0;

    // Turn Shader on
    muir_opengl_shader_switch(shaderProgram);
    muir_GPU_send_variables();

    void * font = GLUT_BITMAP_9_BY_15;
    
    // Texture On!
    glEnable( GL_TEXTURE_RECTANGLE_ARB );

    for(std::vector<Muirgl_Data>::iterator iter = data.begin(); iter != data.end(); iter++)
    {
        double x1 = (iter->radacstart-radac_min)/10000.0;
        double x2 = (iter->radacend-radac_min)/10000.0;
        
        glBindTexture( GL_TEXTURE_RECTANGLE_ARB, iter->texnum );
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, texture_smooth?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, texture_smooth?GL_LINEAR:GL_NEAREST);

        glBegin( GL_QUADS );
        glTexCoord2d(0.0        ,0.0        ); glVertex2d(x1,0.0);
        glTexCoord2d(iter->dataw,0.0        ); glVertex2d(x2,0.0);
        glTexCoord2d(iter->dataw,iter->datah); glVertex2d(x2,iter->datah);
        glTexCoord2d(0.0        ,iter->datah); glVertex2d(x1,iter->datah);
        glEnd();
        
        glPushMatrix();
        glRasterPos2d(x1, 0.0);
        for (std::string::iterator i = iter->filename_decoded.begin(); i != iter->filename_decoded.end(); ++i)
        {
          char c = *i;
          glutBitmapCharacter(font, c);
        }
        glPopMatrix();
    }

    // Texture off
    glDisable( GL_TEXTURE_RECTANGLE_ARB );

#if 0
    glDisable( GL_TEXTURE_2D );
    glColor3f(1.0f,1.0f,1.0f);
    glBegin( GL_QUADS );
    glVertex2d(0.5,0.5);
    glVertex2d(1.0,0.5);
    glVertex2d(1.0,1.0);
    glVertex2d(0.5,1.0);
    glEnd();
#endif

    glPopMatrix();

    glPushMatrix();

    glViewport(0, 0, window_w, window_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_w, 0, window_h, -1, 1);
    //glOrtho(0, window_w, -window_h/2, window_h/2, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    
    // Turn shader off
    muir_opengl_shader_switch(0);

    glColor3f(1.0, 1.0, 1.0); // White

    glRasterPos2f(5, 5);
    std::string s = "Color Scale: Min= " + boost::lexical_cast<std::string>(shader_data_min) + 
                               ", Max= " + boost::lexical_cast<std::string>(shader_data_max);
    
    for (std::string::iterator i = s.begin(); i != s.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    // Find mouse coords relative to opengl coords
    //float mousepos_x = static_cast<float>(mouse_x - window_w/2)/static_cast<float>(window_w) * 3.0f;
    //float mousepos_y = static_cast<float>(-mouse_y + window_h/2)/static_cast<float>(window_h)* 3.0f;

    glRasterPos2f(mouse_x, mouse_y);
    s = "Mouse: " + boost::lexical_cast<std::string>(mouse_x) + "," + boost::lexical_cast<std::string>(mouse_y);
    for (std::string::iterator i = s.begin(); i != s.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glutSwapBuffers();

}


void idleFunc(void)
{
    //Figure out time passage
    double newtime = static_cast<double>(glutGet(GLUT_ELAPSED_TIME))/1000.0;
    double delta_t = newtime - walltime;
    walltime = newtime;

    //std::cout << "Delta Time: " << delta_t << std::endl
    //        << "Time: " << newtime.tv_sec << "." << newtime.tv_nsec << std::endl;

    //Process scoll velocity decay
    scroll_vel += scroll_acc * delta_t;
    if (scroll_vel < 0.0)
        scroll_vel = 0;

    //Process scroll velocity
    x_loc += scroll_vel * delta_t;

    // Render Scene
    glutPostRedisplay();
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

    window_w = w;
    window_h = h;
}

void processNormalKeys(unsigned char key, int x, int y) {

    if (key == 27) 
        exit(0);
    else if (key=='r') {
        int mod = glutGetModifiers();
        //if (mod == GLUT_ACTIVE_ALT)

    }

    if (key == 'q')
    {
        shader_data_min += 0.5f;
        muir_GPU_send_variables();
    }
    if (key == 'a')
    {
        shader_data_min -= 0.5f;
        muir_GPU_send_variables();
    }
    if (key == 'w')
    {
        shader_data_max += 0.5f;
        muir_GPU_send_variables();
    }
    if (key == 's')
    {
        shader_data_max -= 0.5f;
        muir_GPU_send_variables();
    }

}

void processSpecialKeys(int key, int x, int y) {

    int mod;
    switch(key) {
        case GLUT_KEY_F1 : 
            mod = glutGetModifiers();
            if (mod == (GLUT_ACTIVE_CTRL|GLUT_ACTIVE_ALT)) {

            }
            break;
        case GLUT_KEY_F2 : 
            texture_smooth = !texture_smooth;
            //std::cout << "Texture smooth: " << texture_smooth
             break;
        case GLUT_KEY_F3 : 
             break;
	case GLUT_KEY_UP :
             scale *= 1.5;
             x_loc -= (window_w)/(scale*4);
             y_loc -= (window_h)/(scale*4);
             break;
	case GLUT_KEY_DOWN :
             x_loc += (window_w)/(scale*4);
             y_loc += (window_h)/(scale*4);
             scale /= 1.5;
             break;
    }
}


void processMouse(int button, int state, int x, int y) {


//    specialKey = glutGetModifiers();
	// if both a mouse button, and the ALT key, are pressed  then
    if (state == GLUT_DOWN)
    {
        //std::cout << "Button:" << button << std::endl;
        if (button == 3) // scroll up
        {
          scale *= 1.5f;
          x_loc -= (mouse_x)/(scale*2);
          y_loc -= (mouse_y)/(scale*2);
        }

        if (button == 4) // scroll down
        {
          x_loc += (mouse_x)/(scale*2);
          y_loc += (mouse_y)/(scale*2);
          scale /= 1.5f;
        }
    }

std::cout << "Mouse (" << button << ")" << std::endl;

    if (state == GLUT_UP)
    {
        //if (button == 0) // right click
            //scroll_vel = mouse_vel;

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
    y = window_h - y;

    if (x < mouse_x)
       x_loc -= (mouse_x-x)/scale;
    else
       x_loc += (x-mouse_x)/scale;

    mouse_x = x;

    if (y > mouse_y)
        y_loc -= (mouse_y-y)/scale;
    else
        y_loc += (y-mouse_y)/scale;

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
   mouse_y = window_h-y;
}


void processMouseEntry(int state) {
    //if (state == GLUT_LEFT)
    //    deltaAngle = 0.0;/scratch/bellamy/d0000598-decoded.h5
    //else
    //    deltaAngle = 1.0;
}


void LoadHD5Meta(const std::string &filename, std::vector<Muirgl_Data> &datavec )
{
    unsigned int width, height;
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

    // Get radactime data
    Muir2DArrayD radactime;
    h5file.read_2D_double(RTI_DECODEDRADAC_PATH, radactime);

    // texture width
    //width = 512;
    //height = 2048;
    width = dataset_width;
    height = dataset_height;

    for (unsigned int set = 0; set < dataset_count; set++)
    {
        Muirgl_Data dataptr;
        dataptr.texwidth = width;
        dataptr.texheight = height;
        dataptr.datawoffset = 0;
        dataptr.datahoffset = 0;
        dataptr.dataw = dataset_width;
        dataptr.datah = dataset_height;
        dataptr.radacstart = radactime[set][0];
        dataptr.radacend = radactime[set][1];
        dataptr.filename_decoded = filename;

        data = reinterpret_cast<GLfloat*>(malloc( width * height*sizeof(GLfloat) ));

        for (Muir3DArrayF::size_type col = 0; col < width ;col++)
        {
            for (Muir3DArrayF::size_type row = 0; row < height; row++)
            {
                data[row*width + col] = 0.0;
            }
        }

        float min = std::numeric_limits<float>::infinity();
        float max = -std::numeric_limits<float>::infinity();

        for (Muir3DArrayF::size_type col = 0; col < dataset_width ;col++)
        {
            for (Muir3DArrayF::size_type row = 0; row < dataset_height; row++)
            {
                    //float pixel = log10(decoded_data[set][col][row]+1)*10;
                    float pixel = decoded_data[set][col][row];
                    data[row*width + col] = pixel;

                    max = std::max(max, pixel);
                    min = std::min(min, pixel);
           }
        }

        shader_data_max = std::log10(max)*10.0f;
        shader_data_min = std::log10(min+0.01f)*10.0f;
        muir_GPU_send_variables();

        // allocate a texture name
        glGenTextures( 1, &dataptr.texnum );

        // select our current texture
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, dataptr.texnum );

        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);

        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //In this case, the driver will convert your 32 bit float to 16 bit float
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE32F_ARB, width, height, 0, GL_LUMINANCE, GL_FLOAT, data);

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

/// FIXME This is dumb, find a better way
void loadfiles(const std::string &dir)
{
    FS::path path1(dir);

       // If not a command, must be a file
    if (FS::is_directory(path1))
    {

        for (FS::directory_iterator dirI(path1); dirI!=FS::directory_iterator(); ++dirI)
        {
            std::cout << dirI->string() << std::endl;

            if (!FS::is_directory(*dirI))
            {
                try
                {
                    LoadHD5Meta(dirI->string(), data);
                }
                catch(...)
                {
                    std::cout << "Error loading file: " << dirI->string() << std::endl;
                }
               /// FIXME Doesn't scan higher directories
            }
        }
    }
    else
    {
        LoadHD5Meta(dir, data);
    }

    return;
}
