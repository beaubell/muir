
#include "muir-hd5.h"
#include "muir-constants.h"
#include "muir-gl-shader.h"
#include "muir-gl-data.h"
#include "muir-config.h"

#include <iostream>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <ctime>

#ifdef __APPLE__
 #include <glut.h>
 //#include <AGL/agl.h>
 #include <OpenGL/OpenGL.h>
#else
 #include <GL/glut.h>
 #include <GL/glx.h>
#endif

#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

// For loading textures
boost::thread worker_thread;

#include <CL/cl_platform.h>
namespace FS = boost::filesystem;
namespace BSPT = boost::posix_time;


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
#ifdef __APPLE__
void worker_loader(CGLContextObj glcontext);
#else
void worker_loader(Display * display, GLXDrawable drawable, GLXContext glcontext );
#endif
void LoadHD5Meta(const std::string &filename, std::vector<Muirgl_Data*> &data );
void loadfiles(const std::string &dir);


// Global state
int window_w = 0;
int window_h = 0;
double x_loc = 0.0;
double y_loc = 0.0;
const double x_scale_def = 0.0001;
const double y_scale_def = 0.002;
double x_scale = x_scale_def;
double y_scale = y_scale_def;
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
int panel_file_x_min = 500;

// Data Handler
std::vector<Muirgl_Data*> data;

#ifdef __APPLE__
CGLContextObj main_glctx;
#else
GLXContext    main_glctx;
#endif

int main(int argc, char **argv)
{
    std::cout << "MUIR-GL, Version " << PACKAGE_VERSION << std::endl;

    // Initialize timer state
    walltime = static_cast<double>(glutGet(GLUT_ELAPSED_TIME))/1000.0;

    #ifdef __APPLE__
    #else
    XInitThreads();
    #endif
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

    /// Grab OpenGL Context info for multithreading
    #ifdef __APPLE__
    main_glctx = CGLGetCurrentContext();
    CGLEnable( main_glctx, kCGLCEMPEngine);
    #else
    main_glctx = glXGetCurrentContext();
    #endif

    stage_unstage();
    glutMainLoop();

    GLenum err = glGetError();
    if(err)
      std::cout << "OpenGL ERROR: " << err << std::endl;

    return 0;
}


void renderScene(void) {

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    
    glScaled(x_scale, y_scale, 1.0);
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
        //double x1 = (data[i]->radacstart-radac_min)/10000.0;
        //double x2 = (iter->radacend-radac_min)/10000.0;

        data[i]->render(radac_position, texture_smooth);
    }

    // Texture off
    glDisable( GL_TEXTURE_RECTANGLE_ARB );

    glPopMatrix();

    glPushMatrix();
    glScaled(x_scale, 1.0, 1.0);
    glTranslated(x_loc, 0.0, 0.0);
    muir_opengl_shader_switch(0);
    glColor3f(1.0, 1.0, 1.0); // White
    for(unsigned int i = 0; i < data.size(); i++)
    {
        double x1 = (data[i]->radacstart-radac_position);

            glPushMatrix();
            glRasterPos2d(x1, 50.0);
            for (std::string::const_iterator s = data[i]->file_decoded.string().begin(); s != data[i]->file_decoded.string().end(); ++s)
            {
                char c = *s;
                //glutBitmapCharacter(font, c);
            }
            glPopMatrix();
    }

    glPopMatrix();

    glPushMatrix();

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
    std::string s2 = "Position: X= " + boost::lexical_cast<std::string>(radac_min - x_loc) + ", Y= " + boost::lexical_cast<std::string>(y_loc);
    for (std::string::iterator i = s2.begin(); i != s2.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    double cursor_time_us = (radac_min + (-x_loc + mouse_x/x_scale));
    double cursor_time_s = cursor_time_us/1000000.0;
    double cursor_time_frac = (cursor_time_s - std::floor(cursor_time_s))*1000000.0;
    BSPT::ptime curs_time = BSPT::from_time_t((time_t)cursor_time_s);
    curs_time += BSPT::microseconds(cursor_time_frac);

    double cursor_range = (-y_loc + mouse_y/y_scale)/1000.0;
    std::stringstream oss;
    oss << std::setprecision(5) << cursor_range;


    // display mouse coords
    //std::string s3 = "Time : " + boost::lexical_cast<std::string>(cursor_time);
    std::string s4 = BSPT::to_simple_string(curs_time);
    std::string s3 = "Range: " + oss.str() + " km";
    glRasterPos2f(mouse_x+10, mouse_y-30);
    for (std::string::iterator i = s3.begin(); i != s3.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }
    glRasterPos2f(mouse_x+10, mouse_y-48);
    for (std::string::iterator i = s4.begin(); i != s4.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    /// File Panel
    if (panel_file_open)
    {
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.2f,0.5f,0.2f,0.7f);
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
            if ((data[i]->radacstart < (radac_position - (x_loc-window_w/x_scale))) && (data[i]->radacend > (radac_position - (x_loc))))
            {
                glEnable(GL_BLEND);
                glColor4f(0.6f,0.5f,0.7f,0.5f);
                glBegin( GL_QUADS );
                glVertex2d(window_w-panel_file_x_min,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset-4);
                glVertex2d(window_w-panel_file_x_min,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset+14);
                glVertex2d(window_w                 ,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset+14);
                glVertex2d(window_w                 ,(max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset-4);
                glEnd();
                glDisable(GL_BLEND);
            }

            // Print Time
            char gmtbuf[80];
            char lclbuf[80];
            char buf2[200];
            time_t then;
            struct tm *ts;
            then = static_cast<time_t>(data[i]->radacstart/1000000.0);
            ts = gmtime(&then);
            strftime(gmtbuf, sizeof(gmtbuf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
            ts = localtime(&then);
            strftime(lclbuf, sizeof(lclbuf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
            glColor3f(1.0f,1.0f,1.0f);
            glRasterPos2f(window_w-panel_file_x_min+10, (max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset);
            std::string s2 = std::string(gmtbuf) + " (" + data[i]->file_decoded.leaf().string() + ")";

            for (std::string::iterator i = s2.begin(); i != s2.end(); ++i)
            {
                char c = *i;
                glutBitmapCharacter(font, c);
            }

        }
    }

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

    stage_unstage();
    glutPostRedisplay();

}


void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
    if(h == 0)
        h = 1;

    window_w = w;
    window_h = h;
    glViewport(0, 0, window_w, window_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_w, 0, window_h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);

}

void processNormalKeys(unsigned char key, int x, int y)
{
    // Match mouse to gl coords
    y = window_h - y;

    if (key == 27) 
        exit(0);
    else if (key=='r') {
        //int mod = glutGetModifiers();
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

    glutPostRedisplay();
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
             x_scale *= 1.5;
             y_scale *= 1.5;
             x_loc -= (window_w)/(x_scale*4);
             y_loc -= (window_h)/(y_scale*4);
             break;
	case GLUT_KEY_DOWN :
             x_loc += (window_w)/(x_scale*4);
             y_loc += (window_h)/(y_scale*4);
             x_scale /= 1.5;
             y_scale /= 1.5;
             break;
    }
    stage_unstage();
    glutPostRedisplay();
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
                size_t index = ((window_h-y-y_offset + panel_file_scroll_offset*10)/18); // (max_entires-1-i)*18 + panel_file_scroll_offset*10 + y_offset

                if(data.size() > index)
                {
                    //scale = 1;
                    x_loc = 0;
                    y_loc = 0;
                    radac_position = data[index]->radacstart;
                }
                
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
                x_scale *= 1.5f;
                y_scale *= 1.5f;
                x_loc -= (mouse_x)/(x_scale*2);
                y_loc -= (mouse_y)/(y_scale*2);
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
                x_loc += (mouse_x)/(x_scale*2);
                y_loc += (mouse_y)/(y_scale*2);
                x_scale /= 1.5f;
                y_scale /= 1.5f;
            }
        }
        

    }

//std::cout << "Mouse (" << button << ")" << std::endl;

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
    stage_unstage();
    glutPostRedisplay();
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
       x_loc -= (mouse_x-x)/x_scale;
    else
       x_loc += (x-mouse_x)/x_scale;

    mouse_x = x;

    if (y > mouse_y)
        y_loc -= (mouse_y-y)/y_scale;
    else
        y_loc += (y-mouse_y)/y_scale;

    mouse_y = y;
    //stage_unstage();
    glutPostRedisplay();
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

   stage_unstage();
   glutPostRedisplay();
}


void processMouseEntry(int state) {
    //if (state == GLUT_LEFT)
    //    deltaAngle = 0.0;/scratch/bellamy/d0000598-decoded.h5
    //else
    //    deltaAngle = 1.0;
}

boost::condition_variable cond;
boost::mutex mut;

void stage_unstage()
{

    if (!worker_thread.joinable())
    {
        #ifdef __APPLE__
        worker_thread = boost::thread(worker_loader, main_glctx);
        #else
        worker_thread = boost::thread(worker_loader, glXGetCurrentDisplay(), glXGetCurrentDrawable(), main_glctx);
        #endif
    }

    cond.notify_one();

    return;
}

#ifdef __APPLE__
//void worker_loader(AGLContext glcontext)
void worker_loader(CGLContextObj glcontext)
#else
void worker_loader(Display* display, GLXDrawable drawable, GLXContext glcontext )
#endif
{
    #ifdef __APPLE__
    //GLint attrib[] = {AGL_RGBA, AGL_DOUBLEBUFFER, AGL_NONE};
    //AGLPixelFormat aglPixFmt = aglChoosePixelFormat (NULL, 0, attrib);

    //AGLContext this_context = aglCreateContext (aglPixFmt, glcontext);
    //aglSetCurrentContext(this_context);
    CGLPixelFormatAttribute attrib[] = {kCGLPFAAccelerated, (CGLPixelFormatAttribute)0};
    CGLPixelFormatObj pixelFormat = NULL;
    GLint numPixelFormats = 0;

    CGLContextObj this_context = NULL;
    CGLChoosePixelFormat (attrib, &pixelFormat, &numPixelFormats);
    CGLCreateContext(pixelFormat, glcontext, &this_context);

    CGLSetCurrentContext(this_context);
    #else
    static int attrListDbl[] =
    {
        GLX_RGBA,    None
    };

    XVisualInfo * vi = glXChooseVisual( display, 0, attrListDbl);
    GLXContext this_context = glXCreateContext(display, vi, glcontext, GL_TRUE);
    glXMakeCurrent(display, drawable, this_context);
    #endif

    boost::unique_lock<boost::mutex> lock(mut);

    while(1)
    {
        //std::cout << "tesT - " << std::clock() << std::endl;
        cond.wait(lock);
        for(unsigned int i = 0; i < data.size(); i++)
        {

            if ((data[i]->radacstart < (radac_position - (x_loc-window_w/x_scale) )) && (data[i]->radacend > (radac_position - (x_loc) )))
                data[i]->stage();

            if ((data[i]->radacstart > (radac_position - (x_loc-window_w/x_scale*3) )) || (data[i]->radacend < (radac_position - (x_loc+window_w/x_scale*2) )))
                data[i]->release();

        }

    }

    #ifdef __APPLE__
    #else
    glXMakeCurrent(display, drawable, NULL);
    glXDestroyContext(display, this_context);
    #endif
    worker_thread.detach();
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
