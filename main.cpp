#include <iostream>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

Display*                display;
Window                  root;
GLint                   attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
XVisualInfo*            visual;
Colormap                colorMap;
XSetWindowAttributes    setWindowAttribs;
Window                  window;
GLXContext              context;
XWindowAttributes       windowAttribs;
XEvent                  event;

void DrawQuad()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, 1.0, 20.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glBegin(GL_QUADS);
    glColor3f(1.0, 0.0, 0.0); glVertex3f(-0.75, -0.75, 0.0);
    glColor3f(0.0, 1.0, 0.0); glVertex3f(0.75, -0.75, 0.0);
    glColor3f(0.0, 0.0, 1.0); glVertex3f(0.75, 0.75, 0.0);
    glColor3f(1.0, 1.0, 0.0); glVertex3f(-0.75, 0.75, 0.0);
    glEnd();
}

int main()
{
    display = XOpenDisplay(NULL);
    if (display == nullptr)
    {
        std::cout << "Cannot connect to X server!" << std::endl;
        return -1;
    }

    root = DefaultRootWindow(display);

    visual = glXChooseVisual(display, 0, attribs);
    if (visual == nullptr)
    {
        std::cout << "No apprpriat visual found!" << std::endl;
        return -1;
    }
    std::cout << "Visual " << (void*)visual->visualid << " selected." << std::endl;

    colorMap = XCreateColormap(display, root, visual->visual, AllocNone);
    setWindowAttribs.colormap = colorMap;
    setWindowAttribs.event_mask = ExposureMask | KeyPressMask;

    window = XCreateWindow(display, root, 0, 0, 800, 460, 0, visual->depth,
                            InputOutput, visual->visual, CWColormap | CWEventMask, &setWindowAttribs);
    XMapWindow(display, window);
    XStoreName(display, window, "OpenGL and X11 Test");

    context = glXCreateContext(display, visual, NULL, GL_TRUE);
    glXMakeCurrent(display, window, context);
    glEnable(GL_DEPTH_TEST);

    while(true)
    {
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            XGetWindowAttributes(display, window, &windowAttribs);
            glViewport(0, 0, windowAttribs.width, windowAttribs.height);
            DrawQuad();
            glXSwapBuffers(display, window);
        }
        else if (event.type == KeyPress)
        {
            glXMakeCurrent(display, None, NULL);
            glXDestroyContext(display, context);
            XDestroyWindow(display, window);
            XCloseDisplay(display);
            return 0;
        }
    }
    
    return 0;
}