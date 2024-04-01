#ifndef RT_RENDERER_H
#define RT_RENDERER_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Shader.h>
#include <Camera.h>




#include <iostream>
#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <list>

#include "Scene.h"
#include "InputHandler.h"


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
    Camera * GL_camera = nullptr;
    int screen_width = 800;
    int screen_height = 600;
    RenderContext context;// not used for now, we use UBO to pass the camera info

    InputHandler * inputHandler = nullptr;

    PBR_parameters pbr_params;

    GLuint global_ubo;
    UBORenderInfo uploadData;


	Renderer() {
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


        // setup openGL camera
        GL_camera = new Camera(glm::vec3(0, 2, 5));

        // modify camera infos before render loop starts
        GL_camera->MovementSpeed = 1.0f;
        //camera.Front = glm::vec3(-0.373257, -0.393942, -0.826684);
        GL_camera->Front = glm::normalize(glm::vec3(0, 2, 0) - GL_camera->Position);
        GL_camera->ProcessMouseMovement(0, 0); // to update the right and up vector


        // set up input handler
        inputHandler = new InputHandler(window);
        // set up callback functions by registering lambda functions
        // mouse movement
        inputHandler->setMouseMovementCallback([&](double xpos, double ypos) 
        {
            move_camera(xpos, ypos);
        });
        // scroll input
        inputHandler->setScrollCallback([&](double yoffset) 
        {
            adjust_fov(yoffset);
        });
        // frame buffer size change
        inputHandler->setFramebufferSizeCallback([&](int width, int height) 
        {
            adjust_window_size(width, height);
        });
        // keyboard input (not using callback function, but call InputHandler's processKeyboardInput() function in the main loop)
        inputHandler->setKeyboardActionExecution([&]() 
        {
            keyboardActions();
        });




        // set up Dear ImGui context
         //imgui config----------------------
        ImGui_initialize();
        // --------------------------------

        Initialize_CPURT_camera();

        

        InitializeUbo();

        // set up the screenCanvas, it will be used to display the ray tracing result (either CPU version or GPU version)
        screenCanvas = new Rect();


        GPURT_manager = new GPU_RAYTRACER::RaytraceManager(screen_width, screen_height, GL_camera, screenCanvas);

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
        io.Fonts->AddFontFromFileTTF("../../resource/fonts/Cousine-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("../../resource/fonts/DroidSans.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("../../resource/fonts/Karla-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("../../resource/fonts/ProggyClean.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        io.Fonts->AddFontFromFileTTF("../../resource/fonts/Roboto-Medium.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    }

    void Initialize_CPURT_camera() {
        // Setup ray_trace camera
        CPURT_camera = new CPU_RAYTRACER::camera();
        CPURT_camera->aspect_ratio = static_cast<float>(screen_width) / screen_height;
        CPURT_camera->image_width = screen_width;
        CPURT_camera->samples_per_pixel = 10;
        CPURT_camera->max_depth = 8;
        CPURT_camera->vfov = 20;
        CPURT_camera->lookfrom = GL_camera->Position;
        CPURT_camera->lookat = GL_camera->Front;
        CPURT_camera->vup = GL_camera->WorldUp;
        CPURT_camera->defocus_angle = 0; // no defocus blur, fuck it now, I hate it
        CPURT_camera->focus_dist = 10;
    }

    void InitializeUbo() {
        
        glGenBuffers(1, &global_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, global_ubo);

        uploadData.cameraPos = GL_camera->Position;
        uploadData.projection = glm::perspective(glm::radians(GL_camera->Zoom), static_cast<float>(screen_width) / static_cast<float>(screen_height), 0.1f, 100.0f);
        uploadData.view = GL_camera->GetViewMatrix();
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
		uploadData.cameraPos = GL_camera->Position;
		uploadData.projection = glm::perspective(glm::radians(GL_camera->Zoom), static_cast<float>(screen_width) / static_cast<float>(screen_height), 0.1f, 100.0f);
		uploadData.view = GL_camera->GetViewMatrix();
	}



    void update_CPURT_camera() {
		CPURT_camera->lookfrom = GL_camera->Position;
		CPURT_camera->lookat = GL_camera->Position + GL_camera->Front;
        CPURT_camera->vfov = GL_camera->Zoom;
        // when the window size changes
        CPURT_camera->aspect_ratio = static_cast<float>(screen_width) / screen_height;
        CPURT_camera->image_width = screen_width;


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

    //void render

	void render(Scene & _scene, bool render_screenCanvas = false, bool render_ImGUI = true) {
        std::vector<Object*> & objects = _scene.objects;
        std::vector<Model*>& rotate_models = _scene.rotate_models;
		// render loop
		// -----------
        
        // per-frame time logic
        // --------------------
        per_frame_time_logic();

        // render part is here
        // ------

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

            //if (GPURT_manager != nullptr) {
            //    // disable depth test, because we want to draw the screenCanvas on top of the openGL objects
            //    glDisable(GL_DEPTH_TEST);
            //    GPURT_manager->draw_TLAS_AABB();
            //    GPURT_manager->draw_BLAS_AABB();
            //    // enable depth test again
            //    glEnable(GL_DEPTH_TEST);
            //}

            if (render_ImGUI) {
                render_IMGUI();
            }

            return; // if ray tracing is enabled, then the screenCanvas will be updated in the ray tracing thread
            // but we don't need to render the openGL objects in this case, so we return here
        }
        
        
        
        // clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            

        for (auto object : rotate_models) {
            //glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            object->updateRotation(glm::rotate(glm::mat4(1.0f), glm::radians(pbr_params.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)));
		}


        // render the scene using openGL rasterization pipeline
        for (auto object : objects) {
            object->prepareDraw(context);
			object->draw();
		}

        // debug: draw aabb in raytrace_manager
        if (GPURT_manager != nullptr) {
            GPURT_manager->draw_TLAS_AABB();
            //GPURT_manager->draw_BLAS_AABB();
        }

    
        // imgui---------------------------
        if (render_ImGUI){
            render_IMGUI();
        }
        
        // --------------------------------

        
	}


    void per_frame_time_logic() {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1.0f / deltaTime;
        LastTime += deltaTime;



        if (num_frames_in_sliding_window >= num_frames_to_average) {
            frameTime_list.pop_front();
            frameTime_list.push_back(currentFrame);
        }
        else {
            frameTime_list.push_back(fps);
            num_frames_in_sliding_window++;
        }
        average_fps = 1.0f * (num_frames_in_sliding_window - 1) / (frameTime_list.back() - frameTime_list.front());
    }

    void render_IMGUI() {
		// imgui---------------------------
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 240, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(230, 120), ImGuiCond_Always);
        if (ImGui::Begin("LOG", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {

			//ImGui::Text("FPS: %.1f \t AVG_FPS: %.1f", fps, average_fps);
			ImGui::Text("FPS: %.1f ", average_fps);
			ImGui::Text("IS_REALTIME: %s", is_realtime ? "TRUE" : "FALSE");
			ImGui::Text("CAM POS: %.3f %.3f %.3f", GL_camera->Position[0], GL_camera->Position[1], GL_camera->Position[2]);
			ImGui::Text("CAM DIR: %.3f %.3f %.3f", GL_camera->Front[0], GL_camera->Front[1], GL_camera->Front[2]);
			ImGui::Text("CAM FOV: %.3f", GL_camera->Zoom);
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

    void CPURT_render_thread(const Scene& Scene) {
		CPURT_camera->non_blocking_render(Scene.CPURT_objects, CPURT_rendering_finished_flag, &Scene.CPURT_skybox);

        unsigned char* rendered_output = CPURT_camera->rendered_image;
        if (screenCanvas == nullptr) {
            std::cout<<"ERROR: screenCanvas is nullptr"<<std::endl;
            return;
        }
        // no matter how many times this function executes, we always use the same texture (owned by this function)
        // so we don't need to create a new texture every time
        // and GPU ray tracer will use another texture to display the result
        if (CPU_rendered_texture == 0) {
            glGenTextures(1, &CPU_rendered_texture);
        }
        screenCanvas->setTexture(CPU_rendered_texture);// first declear this empty texture to be used by the screenCanvas
        screenCanvas->setTexture(rendered_output, screen_width, screen_height,4); // specify its structure info
        screenCanvas->setShader(new Shader("../../shaders/texture_display.vert", "../../shaders/texture_display.frag"));
	}

    void set_mouse_input(bool enable) {
        enable_mouse_input = enable;
    }
    void set_keyboard_input(bool enable) {
		enable_keyboard_input = enable;
	}
    void set_camera_movement(bool enable) {
        enable_camera_movement = enable;
    }

    bool has_RT_render_request_flag() {
		return CPURT_render_request_flag;
	}
    bool has_reset_request_flag() {
        return default_render_request_flag;
    }
    bool has_rendering_finished_flag() {
		return CPURT_rendering_finished_flag;
	}
    bool has_RT_switch_to_GPU_flag() {
        return GPURT_render_request_flag;
    }
    void reset_RT_render_request_flag() {
        CPURT_render_request_flag = false;
    }
    void reset_reset_request_flag() {
		default_render_request_flag = false;
	}
    void reset_rendering_finished_flag() {
        CPURT_rendering_finished_flag = false;
    }
    void reset_RT_switch_to_GPU_flag() {
        GPURT_render_request_flag = false;
    }

    void updateCPUTexture() {
        unsigned char* rendered_output = CPURT_camera->rendered_image;
        screenCanvas->setTexture(CPU_rendered_texture);
        screenCanvas->updateTexture(rendered_output, screen_width, screen_height, 4);
        context.flipYCoord = true; // because our CPU ray tracing image is flipped (origin at top-left corner, while openGL is at bottom-left corner)
    }


    private:
    //----------------------------------------------------------------------------------
    const bool _vSync = true; // Enable vsync

    // some events flag
    bool CPURT_render_request_flag = false;
    bool default_render_request_flag = false;
    bool CPURT_rendering_finished_flag = false;
    bool GPURT_render_request_flag = false;

    // some control flags
    bool enable_mouse_input = true;
    bool enable_keyboard_input = true;
    bool enable_camera_movement = true;

    // input callback functions
    void adjust_window_size(int width, int height);
    void move_camera(double xpos, double ypos);
    void adjust_fov(double yoffset);
    void keyboardActions();

    // mouse movement variables
    float lastX = screen_width / 2.0f;
    float lastY = screen_height / 2.0f;
    bool firstMouse = true;

    // timing
    float deltaTime = 0.0f;	// time between current frame and last frame
    float lastFrame = 0.0f;
    float LastTime = 0.0f;
    bool is_realtime = true; // the current simulation is in real time or not
    float average_fps = 0.0f;
    float sliding_deltaTime = 0.0f;
    const int num_frames_to_average = 100;
    int num_frames_in_sliding_window = 0;
    std::list<float> frameTime_list;

    // key press
    bool isSpaceKeyPressed = false;
    bool isRightKeyPressed = false;
    bool isDownKeyPressed = false;

    // CPU ray tracing camera output texture
    GLuint CPU_rendered_texture = 0;

};


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Renderer::adjust_window_size(int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    screen_width = width;
    screen_height = height;
    updateUboData();
    // update the ray tracing camera
    update_CPURT_camera();
    // reset the frame count for GPU ray tracing
    if (GPURT_manager != nullptr) {
        GPURT_manager->resetFrameCounter();
        GPURT_manager->changeScreenSize(width, height);
    }
    
}



// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void Renderer::move_camera(double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;
    if (enable_mouse_input) {
        GL_camera->ProcessMouseMovement(xoffset, yoffset);
        if(this->GPURT_manager != nullptr) {
            this->GPURT_manager->resetFrameCounter();
        }
    }
}




// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void Renderer::adjust_fov(double yoffset)
{
    if (enable_mouse_input){
        GL_camera->ProcessMouseScroll(static_cast<float>(yoffset));
        if (this->GPURT_manager != nullptr) {
            this->GPURT_manager->resetFrameCounter();
        }
    }
}

void Renderer::keyboardActions()
{
    if (enable_keyboard_input){
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (enable_camera_movement) {
            bool camera_moved = false;
	    	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
                GL_camera->ProcessKeyboard(FORWARD, deltaTime);
                camera_moved = true;
            }
	    		
	    	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
                GL_camera->ProcessKeyboard(BACKWARD, deltaTime);
                camera_moved = true;
            }
	    	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
                GL_camera->ProcessKeyboard(LEFT, deltaTime);
                camera_moved = true;
            }
	    	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
                GL_camera->ProcessKeyboard(RIGHT, deltaTime);
                camera_moved = true;
            }
	    	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
                GL_camera->ProcessKeyboard(UP, deltaTime);
                camera_moved = true;
            }
	    	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
                GL_camera->ProcessKeyboard(DOWN, deltaTime);
                camera_moved = true;
            }
            if (camera_moved) {
                if(this->GPURT_manager != nullptr) {
                    this->GPURT_manager->resetFrameCounter();
                }
            }
	    }

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            set_camera_movement(false);
            set_mouse_input(false);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	    }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            set_camera_movement(true);
            set_mouse_input(true);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            CPURT_render_request_flag = true;
        }
        else {
            CPURT_render_request_flag = false;
        }

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            default_render_request_flag = true;
        }
        else {
            default_render_request_flag = false;
	    }

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            GPURT_render_request_flag = true;
        }
        else {
            GPURT_render_request_flag = false;
        }

        

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            if (!isRightKeyPressed) {

            }
            isRightKeyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            if (!isDownKeyPressed) {

            }

            isDownKeyPressed = true;
        }
        else {
            isDownKeyPressed = false;
        }
    }
}








#endif