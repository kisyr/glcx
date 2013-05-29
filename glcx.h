#ifndef __GLCX_H
#define __GLCX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <GL/gl.h>

#define __GLCX_LOG(fmt, ...) fprintf(stderr, "glcx: " fmt "\n", ##__VA_ARGS__)

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
    GLCX_KEY_ESCAPE = 128,
};

enum 
{
    GLCX_EVENT_CLOSE = 1000,
    GLCX_EVENT_KEYDOWN,
    GLCX_EVENT_KEYUP,
    GLCX_EVENT_BUTTONDOWN,
    GLCX_EVENT_RESIZE,
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
    GLint resize[2];
} GLCXevent;

static inline GLboolean glcxChooseConfig(const GLint*, GLCXconfig*, GLint, GLint*);

static inline GLCXcontext glcxCreateContext(const GLCXconfig);
static inline GLboolean glcxDestroyContext(GLCXcontext);
static inline GLboolean glcxMakeCurrent(GLCXdrawable, GLCXcontext);

static inline GLCXwindow glcxCreateWindow(const GLCXconfig, const GLint*);
static inline GLboolean glcxDestroyWindow(GLCXwindow);
static inline GLboolean glcxSwapBuffers(GLCXwindow);
static inline GLboolean glcxPollEvent(GLCXevent*);
static inline GLboolean glcxPostEvent(GLCXwindow, GLenum, GLint);

static inline GLCXpbuffer glcxCreatePbuffer(const GLCXconfig, const GLint*);
static inline GLboolean glcxDestroyPbuffer(GLCXpbuffer);

static inline GLboolean glcxShouldQuit();

void* glcxMalloc(size_t sz)
{
    void* mem = malloc(sz);
    memset(mem, 0, sz);
    return mem;
}

#define __GLCX_WINDOW_LIST_SIZE 256
GLCXwindow glcx_window_list[__GLCX_WINDOW_LIST_SIZE] = {0};

void glcxPushWindow(GLCXwindow window)
{
    size_t i;

    for(i = 0; i < __GLCX_WINDOW_LIST_SIZE; ++i) {
        if(!glcx_window_list[i]) {
            glcx_window_list[i] = window;
            break;
        }
    }
}

void glcxPopWindow(GLCXwindow window)
{
    size_t i;

    for(i = 0; i < __GLCX_WINDOW_LIST_SIZE; ++i) {
        if(glcx_window_list[i] == window) {
            glcx_window_list[i] = NULL;
            break;
        }
    }
}

GLboolean glcxShouldQuit()
{
    size_t i;

    for(i = 0; i < __GLCX_WINDOW_LIST_SIZE; ++i) {
        if(glcx_window_list[i])
            return GL_FALSE;
    }

    return GL_TRUE;
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
    GLint size[2];
};

struct _GLCXpbuffer
{
    GLenum type;
    GLXPbuffer object;
};

Display* glcx_display = NULL;
#if 0
#define __GLCX_WINDOW_LIST_SIZE 256
GLCXwindow glcx_window_list[__GLCX_WINDOW_LIST_SIZE] = {0};

void glcxPushWindow(GLCXwindow window)
{
    size_t i;

    for(i = 0; i < __GLCX_WINDOW_LIST_SIZE; ++i) {
        if(!glcx_window_list[i]) {
            glcx_window_list[i] = window;
            break;
        }
    }
}

void glcxPopWindow(GLCXwindow window)
{
    size_t i;

    for(i = 0; i < __GLCX_WINDOW_LIST_SIZE; ++i) {
        if(glcx_window_list[i] == window) {
            glcx_window_list[i] = NULL;
            break;
        }
    }
}
#endif
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

    if(!glcx_display) {
        glcx_display = XOpenDisplay(NULL);
    }

    if(glXQueryVersion(glcx_display, &ver_maj, &ver_min) == False) {
        __GLCX_LOG("glXQueryVersion failed");
        return GL_FALSE;
    }
    if(ver_maj < 2 && ver_min < 3) {
        __GLCX_LOG("GLX version %d.%d < 1.3", ver_maj, ver_min);
        return GL_FALSE;
    }

    for(attribute = attributes, glx_attribute = glx_attributes; 
    attribute && *attribute != GLCX_ATTR_NONE; 
    ++attribute, ++glx_attribute) {
        switch(*attribute) {
        case GLCX_ATTR_TRUE:         *glx_attribute = True;             break;
        case GLCX_ATTR_FALSE:        *glx_attribute = False;            break;
        case GLCX_ATTR_DOUBLEBUFFER: *glx_attribute = GLX_DOUBLEBUFFER; break;
        // NOTE: Is this behaviour necceseary?
        default:                     *glx_attribute = *attribute;       break;
        }
    }

    glx_configs = glXChooseFBConfig(
        glcx_display, 
        DefaultScreen(glcx_display), 
        glx_attributes, 
        &glx_num_configs);

    if(!glx_configs) {
        __GLCX_LOG("glXChooseFBConfig");
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
        __GLCX_LOG("glXCreateContext failed");
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
    GLXDrawable glx_drawable;

    switch(*type) {
    case GLCX_TYPE_WINDOW:  glx_drawable = ((GLCXwindow)drawable)->object;  break;
    case GLCX_TYPE_PBUFFER: glx_drawable = ((GLCXpbuffer)drawable)->object; break;
    default:                return GL_FALSE;
    }

    if(glXMakeContextCurrent(
        glcx_display,
        glx_drawable, 
        glx_drawable, 
        context->object) != True) {
        __GLCX_LOG("glXMakeContextCurrent failed");
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLCXwindow glcxCreateWindow(const GLCXconfig config, const GLint* attributes)
{
    GLCXwindow window = glcxMalloc(sizeof(struct _GLCXwindow));
    const GLint* attribute;

    window->type = GLCX_TYPE_WINDOW;
    window->size[0] = 640;
    window->size[1] = 480;

    for(attribute = attributes; 
    attribute && *attribute != GLCX_ATTR_NONE; 
    ++attribute) {
        switch(*attribute) {
        case GLCX_ATTR_WIDTH:  window->size[0] = *(attribute + 1); break;
        case GLCX_ATTR_HEIGHT: window->size[1] = *(attribute + 1); break;
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
        window->size[0], window->size[1],
        0, 
        visual->depth,
        InputOutput,
        visual->visual,
        CWBorderPixel | CWBackPixel | CWColormap | CWEventMask,
        &swa);
    if(!window->object) {
        __GLCX_LOG("XCreateWindow failed");
        glcxDestroyWindow(window);
        return NULL;
    }

    window->wm_delete = XInternAtom(glcx_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(glcx_display, window->object, &window->wm_delete, 1);

    XFree(visual);
    XStoreName(glcx_display, window->object, "GLCX Window");
    XMapWindow(glcx_display, window->object);

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

    return GL_TRUE;
}

GLCXpbuffer glcxCreatePbuffer(const GLCXconfig config, const GLint* attributes)
{
    GLCXpbuffer pbuffer = glcxMalloc(sizeof(struct _GLCXpbuffer));
    GLint glx_attributes[256] = { None };
    GLint* glx_attribute;
    const GLint* attribute;

    pbuffer->type = GLCX_TYPE_PBUFFER;

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
        __GLCX_LOG("glXCreatePbuffer failed");
        glcxDestroyPbuffer(pbuffer);
        return NULL;
    }

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

        for(i = 0; i < __GLCX_WINDOW_LIST_SIZE; ++i) {
            if(glcx_window_list[i] 
            && glcx_window_list[i]->object == glx_event.xany.window) {
                event->window = glcx_window_list[i];
                break;
            }
        }

        if(!event->window) {
            __GLCX_LOG("glcxPollEvent null window; event: %d", glx_event.type);
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
        case ConfigureNotify:
            if(glx_event.xconfigure.width != event->window->size[0]
            || glx_event.xconfigure.height != event->window->size[1]) {
                event->type = GLCX_EVENT_RESIZE;
                event->resize[0] = event->window->size[0] = glx_event.xconfigure.width;
                event->resize[1] = event->window->size[1] = glx_event.xconfigure.height;
            }
            break;
        }

        return GL_TRUE;
    }

    return GL_FALSE;
}

#endif

/*****************************************************************************
 * WGL implementation                                                        *
 *****************************************************************************/

#ifdef __GLCX_WGL

#include <windows.h>

#define __GLCX_WINDOW_CLASS_NAME "GLCX_WINDOW"
#define __GLCX_MSG_CLOSE WM_USER+1

struct _GLCXconfig
{
    PIXELFORMATDESCRIPTOR pixel_format_desc;
    GLint pixel_format;
};

struct _GLCXcontext
{
    HGLRC object;
};

struct _GLCXwindow
{
    GLenum type;
    HWND object;
    GLint size[2];
    GLboolean wm_close;
};

struct _GLCXpbuffer
{
    GLenum type;
};

static inline LRESULT CALLBACK glcxWindowProc(
    HWND window,
    UINT message,
    WPARAM wparam,
    LPARAM lparam)
{
    GLCXwindow glcx_window;

    switch(message) {
    case WM_CREATE:
        SetWindowLongPtr(
            window, 
            GWLP_USERDATA, 
            (LONG)((LPCREATESTRUCT)lparam)->lpCreateParams);
        break;
    case WM_CLOSE:
        PostMessage(window, __GLCX_MSG_CLOSE, 0, 0);
        return 0;
    }

    return DefWindowProc(window, message, wparam, lparam);
}

static inline ATOM glcxGetWindowClass()
{
    static ATOM window_atom = 0;
    WNDCLASS window_class = {0};

    if(!window_atom) {
        window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = glcxWindowProc;
        window_class.hInstance = GetModuleHandle(NULL);
        window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
        window_class.lpszClassName = __GLCX_WINDOW_CLASS_NAME;

        window_atom = RegisterClass(&window_class);

        if(!window_atom) {
            __GLCX_LOG("RegisterClass failed; error code %d", GetLastError());
        }
    }

    return window_atom;
}

static inline HWND glcxGetDummyWindow()
{
    static HWND dummy_window = 0;

    if(!dummy_window) {
        if(!glcxGetWindowClass()) {
            return GL_FALSE;
        }

        dummy_window = CreateWindow(
            __GLCX_WINDOW_CLASS_NAME,
            "GLCX WINDOW",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            640,
            480,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL);

        if(!dummy_window) {
            __GLCX_LOG("CreateWindow failed; error code %d", GetLastError());
            return GL_FALSE;
        }
    }

    return dummy_window;
}

GLboolean glcxChooseConfig(
    const GLint* attributes,
    GLCXconfig* configs,
    GLint configs_size,
    GLint* configs_num)
{
    PIXELFORMATDESCRIPTOR pixel_format_desc = {0};
    GLint pixel_format;

    if(!glcxGetDummyWindow()) {
        return GL_FALSE;
    }

    pixel_format_desc.nSize = sizeof(pixel_format_desc);
    pixel_format_desc.nVersion = 1;
    pixel_format_desc.dwFlags = PFD_DRAW_TO_WINDOW 
                | PFD_SUPPORT_OPENGL 
                | PFD_DOUBLEBUFFER;
    pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
    pixel_format_desc.cColorBits = 24;
    pixel_format_desc.cDepthBits = 32;
    
    pixel_format = ChoosePixelFormat(
        GetDC(glcxGetDummyWindow()), 
        &pixel_format_desc);

    if(!pixel_format) {
        __GLCX_LOG("ChoosePixelFormat failed; error code %d", GetLastError());
        return GL_FALSE;
    }

    configs[0].pixel_format_desc = pixel_format_desc;
    configs[0].pixel_format = pixel_format;

    *configs_num = 1;

    return GL_TRUE;
}

GLCXcontext glcxCreateContext(const GLCXconfig config)
{
    GLCXcontext context = glcxMalloc(sizeof(struct _GLCXcontext));

    if(!SetPixelFormat(
        GetDC(glcxGetDummyWindow()),
        config.pixel_format,
        &config.pixel_format_desc)) {
        __GLCX_LOG("SetPixelFormat failed; error code %d", GetLastError());
        glcxDestroyContext(context);
        return NULL;
    }

    context->object = wglCreateContext(GetDC(glcxGetDummyWindow()));
    if(!context->object) {
        __GLCX_LOG("wglCreateContext failed; error code %d", GetLastError());
        glcxDestroyContext(context);
        return NULL;
    }

    return context;
}

GLboolean glcxDestroyContext(GLCXcontext context)
{
    if(context->object) {
        wglDeleteContext(context->object);
    }
    free(context);

    return GL_TRUE;
}

GLboolean glcxMakeCurrent(GLCXdrawable drawable, GLCXcontext context)
{
    GLenum* type = (GLenum*)drawable;
    HDC dc;
    HGLRC glrc = context->object;

    switch(*type) {
    case GLCX_TYPE_WINDOW:  dc = GetDC(((GLCXwindow)drawable)->object);  break;
    case GLCX_TYPE_PBUFFER: return GL_FALSE;
    default:                return GL_FALSE;
    }

    if(wglMakeCurrent(dc, glrc) != TRUE) {
        __GLCX_LOG("wglMakeCurrent failed; error code %d", GetLastError());
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLCXwindow glcxCreateWindow(const GLCXconfig config, const GLint* attributes)
{
    GLCXwindow window = glcxMalloc(sizeof(struct _GLCXwindow));
    const GLint* attribute;

    window->type = GLCX_TYPE_WINDOW;
    window->size[0] = 640;
    window->size[1] = 480;

    for(attribute = attributes; 
    attribute && *attribute != GLCX_ATTR_NONE; 
    ++attribute) {
        switch(*attribute) {
        case GLCX_ATTR_WIDTH:  window->size[0] = *(attribute + 1); break;
        case GLCX_ATTR_HEIGHT: window->size[1] = *(attribute + 1); break;
        }
    }
    
    window->object = CreateWindow(
        __GLCX_WINDOW_CLASS_NAME,
        "GLCX WINDOW",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window->size[0],
        window->size[1],
        NULL,
        NULL,
        GetModuleHandle(NULL),
        window);

    if(!window->object) {
        __GLCX_LOG("CreateWindow failed; error code %d", GetLastError());
        glcxDestroyWindow(window);
        return NULL;
    }

    if(!SetPixelFormat(
        GetDC(window->object),
        config.pixel_format,
        &config.pixel_format_desc)) {
        __GLCX_LOG("SetPixelFormat failed; error code %d", GetLastError());
        glcxDestroyWindow(window);
        return NULL;
    }

    UpdateWindow(window->object);
    ShowWindow(window->object, TRUE);

    glcxPushWindow(window);

    return window;
}

GLboolean glcxDestroyWindow(GLCXwindow window)
{
    glcxPopWindow(window);

    if(window->object) {
        DestroyWindow(window->object);
    }
    free(window);

    return GL_TRUE;
}

GLboolean glcxSwapBuffers(GLCXwindow window)
{
    return SwapBuffers(GetDC(window->object)) == TRUE ? GL_TRUE : GL_FALSE;
}

GLboolean glcxPollEvent(GLCXevent* event)
{
    MSG msg;
    size_t i;
    GLCXwindow glcx_window = NULL;

    if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        for(i = 0; i < __GLCX_WINDOW_LIST_SIZE; ++i) {
            if(glcx_window_list[i] 
            && glcx_window_list[i]->object == msg.hwnd) {
                glcx_window = glcx_window_list[i];
                break;
            }
        }

        if(glcx_window && msg.message == __GLCX_MSG_CLOSE) {
            event->type = GLCX_EVENT_CLOSE;
            event->window = glcx_window;
        } else {
            DispatchMessage(&msg);
            TranslateMessage(&msg);
        }

        return GL_TRUE;
    }

    return GL_FALSE;
}

GLboolean glcxPostEvent(
    GLCXwindow window, 
    GLenum message,
    GLint param)
{
    UINT native_message;
    LPARAM native_param;

    switch(message) {
    case GLCX_EVENT_CLOSE:  native_message = WM_CLOSE;  break;
    }

    PostMessage(window->object, native_message, native_param, 0);

    return GL_TRUE;
}

#endif

#ifdef __cplusplus
}
#endif

#endif

