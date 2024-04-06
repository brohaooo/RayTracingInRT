#ifndef RT_RENDERER_H
#define RT_RENDERER_H

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

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"





struct PBR_parameters {
	float eta = 0.5f;
	float m = 0.5f;
    float rotationAngle = 0.0f;
    int render_mode = 0;// 0: full lighting, 1: fresnel only, 2: distribution only, 3: geometric only
    glm::vec3 ka_color = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 kd_color = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 ks_color = glm::vec3(0.5f, 0.5f, 0.5f);
};

struct UBORenderInfo {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 cameraPos;
    float eta; // it helps cameraPos padded to 16 bytes
    float m;
    int render_mode;
    float padding1[2];
    glm::vec3 ka_color;
    float padding2;
    glm::vec3 kd_color;
    float padding3;
    glm::vec3 ks_color;
    float padding4;
};


class Renderer {
    public:

    GLFWwindow * window = nullptr;
    Shader * shader = nullptr;
    Rect * screenCanvas = nullptr;
    CPU_RAYTRACER::camera * CPURT_camera = nullptr;
    GPU_RAYTRACER::RaytraceManager * GPURT_manager = nullptr;
    Camera * camera = nullptr;
    int screen_width = 800;
    int screen_height = 600;
    RenderContext context;// not used for now, we use UBO to pass the camera info
    FrameRateMonitor * frameRateMonitor = nullptr;


    PBR_parameters pbr_params;

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

        // set up Dear ImGui context
         //imgui config----------------------
        ImGui_initialize();
        // --------------------------------

        InitializeUbo();
        // set up the screenCanvas, it will be used to display the ray tracing result (either CPU version or GPU version)
        screenCanvas = new Rect();

	}

    void ImGui_initialize() {
        // set up Dear ImGui context
         //imgui config----------------------
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        // set color theme
        ImGui::StyleColorsDark();
        // embed imgui into glfw and opengl3
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        // OpenGL version 3.3
        ImGui_ImplOpenGL3_Init("#version 330");
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        // font setting
        io.Fonts->AddFontFromFileTTF("resource/fonts/Cousine-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("resource/fonts/DroidSans.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("resource/fonts/Karla-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("resource/fonts/ProggyClean.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("resource/fonts/Roboto-Medium.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    }

    void InitializeUbo() {
        
        glGenBuffers(1, &global_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, global_ubo);

        uploadData.cameraPos = camera->Position;
        uploadData.projection = glm::perspective(glm::radians(camera->Zoom), static_cast<float>(screen_width) / static_cast<float>(screen_height), 0.1f, 100.0f);
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
		uploadData.projection = glm::perspective(glm::radians(camera->Zoom), static_cast<float>(screen_width) / static_cast<float>(screen_height), 0.1f, 100.0f);
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
        uploadData.eta = pbr_params.eta;
        uploadData.m = pbr_params.m;
        uploadData.render_mode = pbr_params.render_mode;
        uploadData.ka_color = pbr_params.ka_color;
        uploadData.kd_color = pbr_params.kd_color;
        uploadData.ks_color = pbr_params.ks_color;

        // upload the UBO data
        uploadUbo();
        
        // if CPU-raytracing or GPU-raytracing is on, we do this 
        if (render_screenCanvas && screenCanvas != nullptr) {
            // for a comparison visualization
            // we preserve the color buffer, but clear the depth buffer, because we will draw the screenCanvas on top of the openGL objects
            glClear(GL_DEPTH_BUFFER_BIT); 
            screenCanvas->prepareDraw(context);
            screenCanvas->draw();
            if (render_ImGUI) {
                render_IMGUI();
            }

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

        std::vector<std::shared_ptr<RenderComponent>> & renderQueue = _scene.renderQueue;
        // render the scene using openGL rasterization pipeline
        for (auto renderComponent : renderQueue) {
            if (renderComponent->active) {
                renderComponent->Render();
            }
        }
        
    
        // imgui---------------------------
        if (render_ImGUI){
            render_IMGUI();
        }
        
        // --------------------------------

        
	}


    void render_IMGUI() {
		// imgui---------------------------
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 240, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(230, 120), ImGuiCond_Always);
        if (ImGui::Begin("LOG", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {

			ImGui::Text("FPS: %.1f ", frameRateMonitor->getFPS()); ImGui::SameLine(); ImGui::Text("| %.1f ", frameRateMonitor->getAverageFPS());
			ImGui::Text("IS_REALTIME: %s", frameRateMonitor->isRealTime() ? "TRUE" : "FALSE");
			ImGui::Text("CAM POS: %.3f %.3f %.3f", camera->Position[0], camera->Position[1], camera->Position[2]);
			ImGui::Text("CAM DIR: %.3f %.3f %.3f", camera->Front[0], camera->Front[1], camera->Front[2]);
			ImGui::Text("CAM FOV: %.3f", camera->Zoom);
		}
        ImGui::End();
//        if (ImGui::Begin("CONTROL PANEL", nullptr)) {
//
//            ImGui::SliderFloat("Rotation", &pbr_params.rotationAngle, 0.0f, 360.0f);
//            ImGui::SliderFloat("eta", &pbr_params.eta, 0.0f, 1.0f);
//            ImGui::SliderFloat("m", &pbr_params.m, 0.0f, 1.0f);
//            // radio buttons
//            ImGui::RadioButton("Full Lighting", &pbr_params.render_mode, 0); ImGui::SameLine();
//            ImGui::RadioButton("Fresnel", &pbr_params.render_mode, 1); ImGui::SameLine();
//            ImGui::RadioButton("Distribution", &pbr_params.render_mode, 2); ImGui::SameLine();
//            ImGui::RadioButton("Geometric", &pbr_params.render_mode, 3);
//            // color edit picker
//            ImGui::ColorEdit3("ka", (float*)&pbr_params.ka_color);
//            ImGui::ColorEdit3("kd", (float*)&pbr_params.kd_color);
//            ImGui::ColorEdit3("ks", (float*)&pbr_params.ks_color);
//            
//        }
//		  ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		// --------------------------------
	}   



    private:
    //----------------------------------------------------------------------------------
    const bool _vSync = true; // Enable vsync

};












#endif