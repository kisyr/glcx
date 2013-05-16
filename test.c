#include "glcx.h"

int main()
{
    const GLint context_attr[] = { 
	GLCX_ATTR_DOUBLEBUFFER, GLCX_ATTR_TRUE, 
	GLCX_ATTR_NONE };
    const GLint drawable_attr[] = { 
	GLCX_ATTR_WIDTH, 640, 
	GLCX_ATTR_HEIGHT, 480, 
	GLCX_ATTR_NONE };
    const GLint configs_size = 3;
    GLCXconfig configs[configs_size];
    GLint configs_num;
    GLCXcontext context;
    GLCXwindow window_a, window_b;

    glcxChooseConfig(context_attr, configs, configs_size, &configs_num);
    printf("configs_num: %d\n", configs_num);

    context = glcxCreateContext(configs[0]);
    window_a = glcxCreateWindow(configs[0], drawable_attr);
    window_b = glcxCreateWindow(configs[0], drawable_attr);

    glcxMakeCurrent(window_a, context);
    glClearColor(1,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glcxSwapBuffers(window_a);

    glcxMakeCurrent(window_b, context);
    glClearColor(0,1,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glcxSwapBuffers(window_b);

    while(!glcxShouldExit()) {
        GLCXevent event;
        while(glcxPollEvent(&event)) {
            switch(event.type) {
            case GLCX_EVENT_CLOSE:
                glcxDestroyWindow(event.window);
                break;
            case GLCX_EVENT_KEYDOWN:
                printf("window: 0x%p key: %d\n", event.window, event.keydown);
                glcxDestroyWindow(event.window);
                break;
            }
        }
    }

    return 0;
}

