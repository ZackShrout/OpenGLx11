#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <GL/glx.h>
#include <X11/Xlib.h>

typedef GLXContext (*glXCreateContextAttribsARBProc)
    (Display*, GLXFBConfig, GLXContext, Bool, const int*);

int main()
{
    Display* display = 0;
    Window window = 0;

    // Create_display_and_window
    display = XOpenDisplay(0);
    window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                              10, 10,   /* x, y */
                              800, 600, /* width, height */
                              0, 0,     /* border_width, border */
                              0);       /* background */

    // Create_the_modern_OpenGL_context
    static int visualAttribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };

    int numFBC = 0;
    GLXFBConfig *fbc = glXChooseFBConfig(display,
                                         DefaultScreen(display),
                                         visualAttribs, &numFBC);
    if (!fbc) {
        std::cout << "glXChooseFBConfig() failed!" << std::endl;
        return 1;
    }

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB =
        (glXCreateContextAttribsARBProc)
        glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

    if (!glXCreateContextAttribsARB) {
        std::cout << "glXCreateContextAttribsARB() not found!" << std::endl;
        return 1;
    }

    // Set desired minimum OpenGL version
    static int contextAttribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 2,
        None
    };
    // Create modern OpenGL contextS
    GLXContext context = glXCreateContextAttribsARB(display, fbc[0], NULL, true,
                                                contextAttribs);
    if (!context) {
        std::cout << "Failed to create OpenGL context! Exiting." << std::endl;
        return 1;
    }

    // Show_the_window
    XMapWindow(display, window);
    glXMakeCurrent(display, window, context);

    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "OpenGL context created.\nVersion " << major << "." << minor << std::endl;
    std::cout << "Vendor " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer " << glGetString(GL_RENDERER) << std::endl;

    // Application_loop
    bool quit = false;
    while (!quit) {
        while (XPending(display)) {
            XEvent xev;
            XNextEvent(display, &xev);

            if (xev.type == KeyPress) {
                quit = true;
            }
        }

        glClearColor(0.8, 0.6, 0.7, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glXSwapBuffers(display, window);

        usleep(1000 * 10);
    }

    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, context);

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}