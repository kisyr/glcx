#include "glcx.h"

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
    GLCXwindow window_a, window_b;
    GLCXevent event;
    GLboolean success;

    success = glcxChooseConfig(attrs, configs, configs_size, &configs_num);
    if(success != GL_TRUE) {
        return EXIT_FAILURE;
    }

    context = glcxCreateContext(configs[0]);
    if(context == NULL) {
        return EXIT_FAILURE;
    }

    window_a = glcxCreateWindow(configs[0], window_attrs);
    window_b = glcxCreateWindow(configs[0], window_attrs);
    if(window_a == NULL || window_b == NULL) {
        return EXIT_FAILURE;
    }

    if(fill(window_a, context, (GLfloat[4]){1,0,0,1}) != GL_TRUE
    || fill(window_b, context, (GLfloat[4]){0,1,0,1}) != GL_TRUE) {
        printf("fill\n");
        return EXIT_FAILURE;
    }

    while(glcxShouldQuit() == GL_FALSE) {
        if(glcxPollEvent(&event) == GL_TRUE
        && event.type == GLCX_EVENT_CLOSE) {
            glcxDestroyWindow(event.window);
        }
        glcxPostEvent(window_a, GLCX_EVENT_CLOSE, 0);
        glcxPostEvent(window_b, GLCX_EVENT_CLOSE, 0);
    }

    return EXIT_SUCCESS;
}

