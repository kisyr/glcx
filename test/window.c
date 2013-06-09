#include "glcx.h"

#define WINDOWS_SIZE 4

GLboolean fill(GLCXwindow window, GLCXcontext context, GLfloat color[4])
{
    GLboolean success;

    success = glcxMakeCurrent(window, context);
    if(success != GL_TRUE) {
        return GL_FALSE;
    }

    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    if(glGetError() != GL_NO_ERROR) {
        return GL_FALSE;
    }

    success = glcxSwapBuffers(window);
    if(success != GL_TRUE) {
        return GL_FALSE;
    }

    return GL_TRUE;
}

int main()
{
    const GLint attrs[] = {
        GLCX_ATTR_DOUBLEBUFFER, GLCX_ATTR_TRUE,
        GLCX_ATTR_NONE };
    const GLint configs_size = 3;
    GLCXconfig configs[configs_size];
    GLint configs_num;
    const GLint window_attrs[] = {
        GLCX_ATTR_WIDTH, 1024,
        GLCX_ATTR_HEIGHT, 768,
        GLCX_ATTR_NONE };
    GLCXcontext context;
    GLCXwindow windows[WINDOWS_SIZE];
    GLCXevent event;
    GLboolean success;
    size_t i;

    success = glcxChooseConfig(attrs, configs, configs_size, &configs_num);
    if(success != GL_TRUE) {
        return EXIT_FAILURE;
    }

    context = glcxCreateContext(configs[0]);
    if(context == NULL) {
        return EXIT_FAILURE;
    }

    for(i = 0; i < WINDOWS_SIZE; ++i) {
        windows[i] = glcxCreateWindow(configs[0], window_attrs);
        if(windows[i] == NULL) {
            return EXIT_FAILURE;
        }
    }

    for(i = 0; i < WINDOWS_SIZE; ++i) {
        success = fill(
            windows[i], 
            context, 
            (GLfloat[4]){(GLfloat)i / WINDOWS_SIZE, 0,0,1});
        if(success != GL_TRUE) {
            return EXIT_FAILURE;
        }
    }

    while(glcxShouldQuit() == GL_FALSE) {
        if(glcxPollEvent(&event) == GL_TRUE
        && event.type == GLCX_EVENT_CLOSE) {
            success = glcxDestroyWindow(event.window);
            if(success != GL_TRUE) {
                return EXIT_FAILURE;
            }
        }
        for(i = 0; i < WINDOWS_SIZE; ++i) {
            success = glcxPostEvent(windows[i], GLCX_EVENT_CLOSE, 0);
            if(success != GL_TRUE) {
                return EXIT_FAILURE;
            }
        }
    }

    glcxDestroyContext(context);

    return EXIT_SUCCESS;
}

