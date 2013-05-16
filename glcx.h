#ifndef __GLCX_H
#define __GLCX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <GL/gl.h>

/*****************************************************************************
 * API                                                                       *
 *****************************************************************************/

enum 
{
    GLCX_ATTR_FALSE = GL_FALSE,
    GLCX_ATTR_TRUE = GL_TRUE,
    GLCX_ATTR_NONE,
    GLCX_ATTR_DOUBLEBUFFER,
    GLCX_ATTR_WIDTH,
    GLCX_ATTR_HEIGHT,
};

enum
{
    GLCX_KEY_ESCAPE = 100,
};

enum 
{
    GLCX_EVENT_CLOSE = 1000,
    GLCX_EVENT_KEYDOWN,
    GLCX_EVENT_KEYUP,
    GLCX_EVENT_BUTTONDOWN,
};

enum
{
    GLCX_TYPE_WINDOW = 10000,
    GLCX_TYPE_PBUFFER,
};

typedef struct _GLCXconfig GLCXconfig;
typedef struct _GLCXcontext* GLCXcontext;
typedef struct _GLCXwindow* GLCXwindow;
typedef struct _GLCXpbuffer* GLCXpbuffer;
typedef void* GLCXdrawable;

typedef struct
{
    GLenum type;
    GLCXwindow window;
    GLint keydown;
    GLint keyup;
    GLint buttondown;
} GLCXevent;

static inline GLboolean glcxChooseConfig(const GLint*, GLCXconfig*, GLint, GLint*);

static inline GLCXcontext glcxCreateContext(const GLCXconfig);
static inline GLboolean glcxDestroyContext(GLCXcontext);
static inline GLboolean glcxMakeCurrent(GLCXdrawable, GLCXcontext);

static inline GLCXwindow glcxCreateWindow(const GLCXconfig, const GLint*);
static inline GLboolean glcxDestroyWindow(GLCXwindow);
static inline GLboolean glcxSwapBuffers(GLCXwindow);
static inline GLboolean glcxPollEvent(GLCXevent*);

static inline GLCXpbuffer glcxCreatePbuffer(const GLCXconfig, const GLint*);
static inline GLboolean glcxDestroyPbuffer(GLCXpbuffer);

static inline GLboolean glcxShouldExit();

void* glcxMalloc(size_t sz)
{
    void* mem = malloc(sz);
    memset(mem, 0, sz);
    return mem;
}

/*****************************************************************************
 * GLX implementation                                                        *
 *****************************************************************************/

#ifdef __GLCX_GLX

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

struct _GLCXconfig
{
    GLXFBConfig object;
};

struct _GLCXcontext
{
    GLXContext object;
};

struct _GLCXwindow
{
    GLenum type;
    Window object;
    Atom wm_delete;
};

struct _GLCXpbuffer
{
    GLenum type;
    GLXPbuffer object;
};

Display* glcx_display = NULL;
#define glcx_window_list_size 256
GLCXwindow glcx_window_list[glcx_window_list_size] = {0};

void glcxPushWindow(GLCXwindow window)
{
    size_t i;

    for(i = 0; i < glcx_window_list_size; ++i) {
        if(!glcx_window_list[i]) {
            glcx_window_list[i] = window;
            break;
        }
    }
}

void glcxPopWindow(GLCXwindow window)
{
    size_t i;

    for(i = 0; i < glcx_window_list_size; ++i) {
        if(glcx_window_list[i] == window) {
            glcx_window_list[i] = NULL;
            break;
        }
    }
}

GLboolean glcxChooseConfig(
    const GLint* attributes, 
    GLCXconfig* configs,
    GLint configs_size,
    GLint* configs_num)
{
    GLint ver_maj, ver_min;
    GLint glx_attributes[256] = { None };
    GLint* glx_attribute;
    const GLint* attribute;
    GLint glx_num_configs;
    GLXFBConfig* glx_configs;
    GLint i;

    if(!glcx_display)
        glcx_display = XOpenDisplay(NULL);

    if(False == glXQueryVersion(glcx_display, &ver_maj, &ver_min)) {
        fprintf(stderr, "glXQueryVersion\n");
        return GL_FALSE;
    }
    if(ver_maj < 2 && ver_min < 3) {
        fprintf(stderr, "GLX version %d.%d < 1.3\n", ver_maj, ver_min);
        return GL_FALSE;
    }

    for(attribute = attributes, glx_attribute = glx_attributes; 
    attribute && *attribute != GLCX_ATTR_NONE; 
    ++attribute, ++glx_attribute) {
        switch(*attribute) {
        case GLCX_ATTR_TRUE:         *glx_attribute = True;             break;
        case GLCX_ATTR_FALSE:        *glx_attribute = False;            break;
        case GLCX_ATTR_DOUBLEBUFFER: *glx_attribute = GLX_DOUBLEBUFFER; break;
        default:                     *glx_attribute = *attribute;       break;
        }
    }

    glx_configs = glXChooseFBConfig(
        glcx_display, 
        DefaultScreen(glcx_display), 
        glx_attributes, 
        &glx_num_configs);

    if(!glx_configs) {
        fprintf(stderr, "glXChooseFBConfig\n");
        return GL_FALSE;
    }

    for(i = 0; i < glx_num_configs && i < configs_size; ++i) {
        configs[i].object = glx_configs[i];
    }
    *configs_num = glx_num_configs;

    return GL_TRUE;
}

GLCXcontext glcxCreateContext(const GLCXconfig config)
{
    GLCXcontext context = glcxMalloc(sizeof(struct _GLCXcontext));

    //XVisualInfo* visual = glXGetVisualFromFBConfig(glcx_display, config.config);
    //context->context = glXCreateContext(glcx_display, visual, NULL, True);
    context->object = glXCreateNewContext(
        glcx_display, 
        config.object, 
        GLX_RGBA_TYPE,
        NULL,
        True);
    if(!context->object) {
        fprintf(stderr, "glXCreateContext");
        glcxDestroyContext(context);
        return NULL;
    }

    return context;
}

GLboolean glcxDestroyContext(GLCXcontext context)
{
    if(context->object) {
        glXDestroyContext(glcx_display, context->object);
    }
    free(context);

    return GL_TRUE;
}

GLboolean glcxMakeCurrent(GLCXdrawable drawable, GLCXcontext context)
{
    GLenum* type = (GLenum*)drawable;
    GLXDrawable glx_drawable = 0;
    switch(*type) {
    case GLCX_TYPE_WINDOW:  glx_drawable = ((GLCXwindow)drawable)->object;  break;
    case GLCX_TYPE_PBUFFER: glx_drawable = ((GLCXpbuffer)drawable)->object; break;
    }

    return glXMakeContextCurrent(
        glcx_display,
        glx_drawable, 
        glx_drawable, 
        context->object) == True ? GL_TRUE : GL_FALSE;
}

GLCXwindow glcxCreateWindow(const GLCXconfig config, const GLint* attributes)
{
    GLCXwindow window = glcxMalloc(sizeof(struct _GLCXwindow));
    GLint window_size[2] = { 640, 480 };
    const GLint* attribute;

    for(attribute = attributes; 
    attribute && *attribute != GLCX_ATTR_NONE; 
    ++attribute) {
        switch(*attribute) {
        case GLCX_ATTR_WIDTH:  window_size[0] = *(attribute + 1); break;
        case GLCX_ATTR_HEIGHT: window_size[1] = *(attribute + 1); break;
        }
    }

    XVisualInfo* visual = glXGetVisualFromFBConfig(glcx_display, config.object);
    XSetWindowAttributes swa;

    swa.colormap = XCreateColormap(
    glcx_display, 
    RootWindow(glcx_display, visual->screen),
    visual->visual,
    AllocNone);
    //swa.background_pixmap = None;
    swa.background_pixel = BlackPixel(glcx_display, visual->screen);
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask
           | KeyPressMask
           | KeyReleaseMask
           | ButtonPressMask
           | ButtonReleaseMask;

    window->object = XCreateWindow(
        glcx_display,
        RootWindow(glcx_display, visual->screen),
        0, 0, 
        window_size[0], window_size[1],
        0, 
        visual->depth,
        InputOutput,
        visual->visual,
        CWBorderPixel | CWBackPixel | CWColormap | CWEventMask,
        &swa);
    if(!window->object) {
        fprintf(stderr, "XCreateWindow\n");
        glcxDestroyWindow(window);
        return NULL;
    }

    window->wm_delete = XInternAtom(glcx_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(glcx_display, window->object, &window->wm_delete, 1);

    XFree(visual);
    XStoreName(glcx_display, window->object, "GLCX Window");
    XMapWindow(glcx_display, window->object);

    window->type = GLCX_TYPE_WINDOW;

    glcxPushWindow(window);

    return window;
}

GLboolean glcxDestroyWindow(GLCXwindow window)
{
    glcxPopWindow(window);

    if(window->object) {
        XUnmapWindow(glcx_display, window->object);
        XDestroyWindow(glcx_display, window->object);
    }
    free(window);

    return GL_TRUE;
}

GLboolean glcxSwapBuffers(GLCXwindow window)
{
    glXSwapBuffers(glcx_display, window->object);
}

GLCXpbuffer glcxCreatePbuffer(const GLCXconfig config, const GLint* attributes)
{
    GLCXpbuffer pbuffer = glcxMalloc(sizeof(struct _GLCXpbuffer));
    GLint glx_attributes[256] = { None };
    GLint* glx_attribute;
    const GLint* attribute;

    for(attribute = attributes, glx_attribute = glx_attributes;
    attribute && *attribute != GLCX_ATTR_NONE; 
    ++attribute, ++glx_attribute) {
        switch(*attribute) {
        case GLCX_ATTR_WIDTH:  *glx_attribute = GLX_PBUFFER_WIDTH;  break;
        case GLCX_ATTR_HEIGHT: *glx_attribute = GLX_PBUFFER_HEIGHT; break;
        default:               *glx_attribute = *attribute;         break;
        }
    }

    pbuffer->object = glXCreatePbuffer(
        glcx_display,
        config.object,
        glx_attributes);
    if(!pbuffer->object) {
        fprintf(stderr, "glXCreatePbuffer\n");
        glcxDestroyPbuffer(pbuffer);
        return NULL;
    }

    pbuffer->type = GLCX_TYPE_PBUFFER;

    return pbuffer;
}

GLboolean glcxDestroyPbuffer(GLCXpbuffer pbuffer)
{
    if(pbuffer->object) {
        glXDestroyPbuffer(glcx_display, pbuffer->object);
    }
    free(pbuffer);

    return GL_TRUE;
}

GLint glcxTranslateKey(GLint key)
{
    int syms_per_code;
    key = *XGetKeyboardMapping(glcx_display, key, 1, &syms_per_code);

    switch(key) {
    case XK_Escape: return GLCX_KEY_ESCAPE;
    default:        return key;
    }
}

GLboolean glcxPollEvent(GLCXevent* event)
{
    XEvent glx_event;
    size_t i;

    memset(event, 0, sizeof(GLCXevent));

    if(XPending(glcx_display) > 0) {
        XNextEvent(glcx_display, &glx_event);

        for(i = 0; i < glcx_window_list_size; ++i) {
            if(glcx_window_list[i] 
            && glcx_window_list[i]->object == glx_event.xany.window) {
                event->window = glcx_window_list[i];
                break;
            }
        }

        if(!event->window) {
            fprintf(stderr, "glcxPollEvent: null window; event: %d\n", 
                glx_event.type);
            return GL_FALSE;
        }

        switch(glx_event.type) {
        case ClientMessage: 
            if(glx_event.xclient.data.l[0] == event->window->wm_delete) {
                event->type = GLCX_EVENT_CLOSE;
            }
            break;
        case KeyPress:
            event->type = GLCX_EVENT_KEYDOWN;
            event->keydown = glcxTranslateKey(glx_event.xkey.keycode);
            break;
        case KeyRelease:
            event->type = GLCX_EVENT_KEYUP;
            event->keyup = glcxTranslateKey(glx_event.xkey.keycode);
            break;
        }

        return GL_TRUE;
    }

    return GL_FALSE;
}

GLboolean glcxShouldExit()
{
    size_t i;

    for(i = 0; i < glcx_window_list_size; ++i) {
        if(glcx_window_list[i])
            return GL_FALSE;
    }

    return GL_TRUE;
}

#endif

/*****************************************************************************
 * WGL implementation                                                        *
 *****************************************************************************/

#ifdef __GLCX_WGL
#endif

#ifdef __cplusplus
}
#endif

#endif

