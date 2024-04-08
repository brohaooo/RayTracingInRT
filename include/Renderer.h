#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Shader.h>
#include <Camera.h>
#include <Scene.h>
#include <FrameRateMonitor.h>


#include <iostream>
#include <chrono>
#include <list>





struct UBORenderInfo {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 cameraPos;
};


class Renderer {
    public:

    GLFWwindow * window = nullptr;
    Shader * shader = nullptr;
    GRect * screenCanvas = nullptr;
    CPU_RAYTRACER::camera * CPURT_camera = nullptr;
    GPU_RAYTRACER::RaytraceManager * GPURT_manager = nullptr;
    Camera * camera = nullptr;
    int screen_width = 800;
    int screen_height = 600;
    RenderContext * context;
    FrameRateMonitor * frameRateMonitor = nullptr;

    GLuint global_ubo;
    UBORenderInfo uploadData;


	Renderer(int _screen_width, int _screen_height, Camera * camera = nullptr) : camera(camera), screen_width(_screen_width), screen_height(_screen_height) {
		initialize();
	}

	void initialize() {
        //----------------------------------------------------------------------------------
        // glfw: initialize and configure
        // ------------------------------
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // Enable double buffering
        glfwWindowHint(GLFW_SAMPLES, 4); // Enable multisampling



        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif

        // glfw window creation
        // --------------------
        std::cout << "Creating GLFW window" << std::endl;
        window = glfwCreateWindow(screen_width, screen_height, "RTRT", NULL, NULL);


        if (window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
        // set window position and size
        glfwMakeContextCurrent(window);

        glfwSwapInterval(_vSync ? 1 : 0); // Enable vsync

        // tell GLFW to capture our mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // glad: load all OpenGL function pointers
        // ---------------------------------------
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return;
        }


        // configure global opengl state (they are not global, but we treat them as global since we probably won't change them)
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glPointSize(8.0);
        glLineWidth(4.0);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        InitializeUbo();
        // set up the screenCanvas, it will be used to display the ray tracing result (either CPU version or GPU version)
        screenCanvas = new GRect();

        context = new RenderContext();

	}

    void InitializeUbo() {
        
        glGenBuffers(1, &global_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, global_ubo);

        uploadData.cameraPos = camera->Position;
        uploadData.projection = glm::perspective(glm::radians(camera->Zoom), static_cast<float>(screen_width) / static_cast<float>(screen_height), 0.1f, 500.0f);
        uploadData.view = camera->GetViewMatrix();
        glBufferData(GL_UNIFORM_BUFFER, sizeof(uploadData), &uploadData, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        GLuint bindingPoint = 0;
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, global_ubo);
    }

    void uploadUbo() {
        if (global_ubo == 0) {
            glGenBuffers(1, &global_ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, global_ubo);
        }
        else {
            glBindBuffer(GL_UNIFORM_BUFFER, global_ubo);
        }
        glBufferData(GL_UNIFORM_BUFFER, sizeof(uploadData), &uploadData, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void updateUboData() {
		uploadData.cameraPos = camera->Position;
		uploadData.projection = glm::perspective(glm::radians(camera->Zoom), static_cast<float>(screen_width) / static_cast<float>(screen_height), 0.1f, 500.0f);
		uploadData.view = camera->GetViewMatrix();
	}

    void poll_events() {
        glfwPollEvents();
    }

    void swap_buffers() {
		glfwSwapBuffers(window);
	}

    void renderCanvas() {
        screenCanvas->prepareDraw(context);
        screenCanvas->draw();
    }

    void resize(int _screen_width, int _screen_height) {
        // make sure the viewport matches the new window dimensions; note that width and 
        // height will be significantly larger than specified on retina displays.
        glViewport(0, 0, _screen_width, _screen_height);
        screen_width = _screen_width;
        screen_height = _screen_height;
        updateUboData();
        
    }

    // render loop
	void render(Scene & _scene, bool render_screenCanvas = false, bool render_ImGUI = true) {
        // update the UBO data
        updateUboData();

        // upload the UBO data
        uploadUbo();
        
        // if CPU-raytracing or GPU-raytracing is on, we do this 
        if (render_screenCanvas && screenCanvas != nullptr) {
            // for a comparison visualization
            // we preserve the color buffer, but clear the depth buffer, because we will draw the screenCanvas on top of the openGL objects
            glClear(GL_DEPTH_BUFFER_BIT); 
            screenCanvas->prepareDraw(context);
            screenCanvas->draw();

            return; // if ray tracing is enabled, then the screenCanvas will be updated in the ray tracing thread
            // but we don't need to render the openGL objects in this case, so we return here
        }
        
        // clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // debug: draw aabb in raytrace_manager
        if (GPURT_manager != nullptr) {
            GPURT_manager->draw_TLAS_AABB();
            //GPURT_manager->draw_BLAS_AABB();
        }

        // get light info
        std::vector<Light*> & sceneLights = _scene.sceneLights;
        // currently we only support one light and it is a point light
        if (sceneLights.size() > 0) {
            Light * light = sceneLights[0];
            if (light->lightType == POINT_LIGHT) {
                PointLight * pointLight = dynamic_cast<PointLight*>(light);
                context->lightPos = pointLight->position;
                context->lightColor = pointLight->color;
                context->lightCount = sceneLights.size();
                context->ambientColor = glm::vec3(0.4,0.4,0.5);
            }
        }


        std::vector<std::shared_ptr<RenderComponent>> & renderQueue = _scene.renderQueue;
        // render the scene using openGL rasterization pipeline
        for (auto renderComponent : renderQueue) {
            if (renderComponent->active) {
                renderComponent->Render();
            }
        }
        

	}


    private:
    //----------------------------------------------------------------------------------
    const bool _vSync = true; // Enable vsync

};












#endif