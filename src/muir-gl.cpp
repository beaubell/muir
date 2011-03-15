
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-gl-shader.h"
#include "muir-gl-data.h"
#include "muir-config.h"

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
#include <CL/cl_platform.h>
namespace FS = boost::filesystem;


void renderScene(void);
void idleFunc(void);
void changeSize(int w, int h);
void processNormalKeys(unsigned char key, int x, int y);
void processSpecialKeys(int key, int x, int y);

void processMouse(int button, int state, int x, int y);
void processMouseActiveMotion(int x, int y);
void processMousePassiveMotion(int x, int y);
void processMouseEntry(int state);
void stage_unstage();

void LoadHD5Meta(const std::string &filename, std::vector<Muirgl_Data*> &data );
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
float scroll_vel = 0.0f;
float scroll_acc = 0.0f;
bool texture_smooth = true;
double walltime = 0.0;

bool panel_file_open = false;
int panel_file_scroll_offset = 0;
int panel_file_x_min = 250;

// Data Handler
std::vector<Muirgl_Data*> data;



int main(int argc, char **argv)
{
    std::cout << "MUIR-GL, Version " << PACKAGE_VERSION << std::endl;

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
    for (int i = 1; i < argc; i++)
        loadfiles(argv[i]);

    // Find radactime min/max
    radac_max = data[0]->radacend;
    radac_min = data[0]->radacstart;
    for(unsigned int i = 0; i < data.size(); i++)
    {
            radac_max = std::max(data[i]->radacend, radac_max);
            radac_min = std::min(data[i]->radacstart, radac_min);
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

    for(unsigned int i = 0; i < data.size(); i++)
    {
        double x1 = (data[i]->radacstart-radac_min)/10000.0;
        //double x2 = (iter->radacend-radac_min)/10000.0;

        data[i]->render(radac_position, texture_smooth);
    }

    // Texture off
    glDisable( GL_TEXTURE_RECTANGLE_ARB );

    glPopMatrix();

    glPushMatrix();
    glScaled(scale, 1.0, 1.0);
    glTranslated(x_loc, 0.0, 0.0);
    muir_opengl_shader_switch(0);
    glColor3f(1.0, 1.0, 1.0); // White
    for(unsigned int i = 0; i < data.size(); i++)
    {
        double x1 = (data[i]->radacstart-radac_position)/10000.0;

            glPushMatrix();
            glRasterPos2d(x1, 50.0);
            for (std::string::const_iterator s = data[i]->file_decoded.string().begin(); s != data[i]->file_decoded.string().end(); ++s)
            {
                char c = *s;
                glutBitmapCharacter(font, c);
            }
            glPopMatrix();
    }
    
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
    std::string s1 = "Color Scale: Min= " + boost::lexical_cast<std::string>(shader_data_min) + 
                               ", Max= " + boost::lexical_cast<std::string>(shader_data_max);
    
    for (std::string::iterator i = s1.begin(); i != s1.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    glRasterPos2f(5, 20);
    std::string s2 = "Position: X= " + boost::lexical_cast<std::string>(radac_min - x_loc*10000.0) + ", Y= " + boost::lexical_cast<std::string>(data[1]->radacstart);
    for (std::string::iterator i = s2.begin(); i != s2.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    // Find mouse coords relative to opengl coords
    //float mousepos_x = static_cast<float>(mouse_x - window_w/2)/static_cast<float>(window_w) * 3.0f;
    //float mousepos_y = static_cast<float>(-mouse_y + window_h/2)/static_cast<float>(window_h)* 3.0f;

    glRasterPos2f(mouse_x, mouse_y);
    std::string s3 = "Mouse: " + boost::lexical_cast<std::string>(mouse_x) + "," + boost::lexical_cast<std::string>(mouse_y);
    for (std::string::iterator i = s3.begin(); i != s3.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    /// File Panel
    

    if (panel_file_open)
    {
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.2f,0.5f,0.2f,0.5f);
        glBegin( GL_QUADS );
        glVertex2d(window_w-panel_file_x_min,window_h);
        glVertex2d(window_w-panel_file_x_min,0);
        glVertex2d(window_w,0);
        glVertex2d(window_w,window_h);
        glEnd();

        glDisable(GL_BLEND);
        glColor3f(1.0f,1.0f,1.0f);

        int max_entires = window_h/18;
        int y_offset = window_h%18;
        for(unsigned int i = 0; i < data.size(); i++)
        {
            // Highlight entries in view
            if ((data[i]->radacstart < (radac_position - (x_loc-window_w/scale)*10000 )) && (data[i]->radacend > (radac_position - (x_loc)*10000 )))
            {
                glEnable(GL_BLEND);
                glColor4f(0.2f,0.5f,0.7f,0.5f);
                glBegin( GL_QUADS );
                glVertex2d(window_w-panel_file_x_min,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset-3);
                glVertex2d(window_w-panel_file_x_min,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset+15);
                glVertex2d(window_w                 ,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset+15);
                glVertex2d(window_w                 ,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset-3);
                glEnd();
                glDisable(GL_BLEND);
            }

            glColor3f(1.0f,1.0f,1.0f);
            glRasterPos2f(window_w-panel_file_x_min+10, (max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset);
            std::string s1 = data[i]->file_decoded.filename().string();
            
            for (std::string::iterator i = s1.begin(); i != s1.end(); ++i)
            {
                char c = *i;
                glutBitmapCharacter(font, c);
            }
        }
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

    stage_unstage();
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

void processNormalKeys(unsigned char key, int x, int y)
{
    // Match mouse to gl coords
    y = window_h - y;

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

void processSpecialKeys(int key, int x, int y)
{
    // Match mouse to gl coords
    y = window_h - y;

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


void processMouse(int button, int state, int x, int y)
{

    // Match mouse to gl coords
    y = window_h - y;

    
//    specialKey = glutGetModifiers();
	// if both a mouse button, and the ALT key, are pressed  then
    if (state == GLUT_DOWN)
    {
        //std::cout << "Button:" << button << std::endl;
        if (button == 0) // Keft click
        {
            if(panel_file_open)
            {
                int y_offset = window_h%18;
                int index = ((window_h-y-y_offset + panel_file_scroll_offset*10)/18); // (max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset
                
                //scale = 1;
                x_loc = 0;
                //y_loc = 0;
                radac_position = data[index]->radacstart;
                
                
            }
        }
        
        //std::cout << "Button:" << button << std::endl;
        if (button == 3) // scroll up
        {
            if(panel_file_open)
            {
                panel_file_scroll_offset--;

                if(panel_file_scroll_offset < 0)
                    panel_file_scroll_offset = 0;
            }
            else
            {
                scale *= 1.5f;
                x_loc -= (mouse_x)/(scale*2);
                y_loc -= (mouse_y)/(scale*2);
            }
        }

        if (button == 4) // scroll down
        {
            if(panel_file_open)
            {
                panel_file_scroll_offset++;
            }
            else
            {
                x_loc += (mouse_x)/(scale*2);
                y_loc += (mouse_y)/(scale*2);
                scale /= 1.5f;
            }
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

   if ((mouse_x > window_w-30))
       panel_file_open = true;

   if ((mouse_x < window_w-panel_file_x_min))
       panel_file_open = false;
}


void processMouseEntry(int state) {
    //if (state == GLUT_LEFT)
    //    deltaAngle = 0.0;/scratch/bellamy/d0000598-decoded.h5
    //else
    //    deltaAngle = 1.0;
}


void stage_unstage()
{
    for(unsigned int i = 0; i < data.size(); i++)
    {

        if ((data[i]->radacstart < (radac_position - (x_loc-window_w/scale)*10000 )) && (data[i]->radacend > (radac_position - (x_loc)*10000 )))
            data[i]->stage();

        if ((data[i]->radacstart > (radac_position - (x_loc-window_w/scale*3)*10000 )) || (data[i]->radacend < (radac_position - (x_loc+window_w/scale*2)*10000 )))
            data[i]->release();

    }

}


void LoadHD5Meta(const std::string &filename, std::vector<Muirgl_Data *> &datavec )
{


    Muirgl_Data *dataptr = new Muirgl_Data(filename);

    //dataptr->stage();
    datavec.push_back(dataptr);


}

/// FIXME This is dumb, find a better way
void loadfiles(const std::string &dir)
{
    FS::path path1(dir);

       // If not a command, must be a file
    if (FS::is_directory(path1))
    {
        typedef std::vector<FS::path> vec;             // store paths,
        vec v;                                // so we can sort them later
        
        std::copy(FS::directory_iterator(path1), FS::directory_iterator(), std::back_inserter(v));
        
        std::sort(v.begin(), v.end());   // sort, since directory iteration
                                    // is not ordered on some file systems

        for (vec::const_iterator dirI (v.begin()); dirI != v.end(); ++dirI)
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
