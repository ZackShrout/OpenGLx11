#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <GL/glx.h>
#include <X11/Xlib.h>

constexpr int width { 800 };
constexpr int height { 600 };

typedef GLXContext (*glXCreateContextAttribsARBProc)
    (Display*, GLXFBConfig, GLXContext, Bool, const int*);

int main()
{
    // Create window
    Display* display { XOpenDisplay(0) };
    if (display == NULL) {
        std::cout << "Cannot connect to X server!" << std::endl;
        return 1;
    }

    int screen { DefaultScreen(display) };
    Visual* visual { DefaultVisual(display, screen) };
    Colormap colormap { XCreateColormap(display, DefaultRootWindow(display), visual, AllocNone) };

    XSetWindowAttributes attributes;
    attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
                            ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    attributes.colormap = colormap;

     Window window { XCreateWindow(display, DefaultRootWindow(display), 0, 0, width, height, 0,
                                    DefaultDepth(display, screen), InputOutput, visual,
                                    CWColormap | CWEventMask, &attributes) };

    // Create_the_modern_OpenGL_context
    static int visualAttribs[] {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };

    int numFBC { 0 };
    GLXFBConfig *fbc { glXChooseFBConfig(display,
                                         DefaultScreen(display),
                                         visualAttribs, &numFBC) };
    if (!fbc) {
        std::cout << "glXChooseFBConfig() failed!" << std::endl;
        return 1;
    }

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB { 0 };
    glXCreateContextAttribsARB =
        (glXCreateContextAttribsARBProc)
        glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

    if (!glXCreateContextAttribsARB) {
        std::cout << "glXCreateContextAttribsARB() not found!" << std::endl;
        return 1;
    }

    // Set desired minimum OpenGL version
    int contextAttribs[] {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 2,
        None
    };

    // Create modern OpenGL context
    GLXContext context { glXCreateContextAttribsARB(display, fbc[0], NULL, true,
                                                contextAttribs) };
    if (!context) {
        std::cout << "Failed to create OpenGL context! Exiting." << std::endl;
        return 1;
    }

    // Show window
    XMapWindow(display, window);
    XStoreName(display, window, "Modern GLX with X11");
    glXMakeCurrent(display, window, context);

    int major { 0 }, minor { 0 };
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "OpenGL context created.\nVersion " << major << "." << minor << std::endl;
    std::cout << "Vendor " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer " << glGetString(GL_RENDERER) << std::endl;

    // Application_loop
    bool quit { false };
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

    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, context);

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}