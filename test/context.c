#include "glcx.h"

int main()
{
    const GLint attrs[] = {
        GLCX_ATTR_DOUBLEBUFFER, GLCX_ATTR_TRUE,
        GLCX_ATTR_NONE };
    const GLint configs_size = 3;
    GLCXconfig configs[configs_size];
    GLint configs_num;
    GLCXcontext context;
    GLboolean success;

    success = glcxChooseConfig(attrs, configs, configs_size, &configs_num);
    if(success != GL_TRUE) {
        return EXIT_FAILURE;
    }

    context = glcxCreateContext(configs[0]);
    if(context == NULL) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

