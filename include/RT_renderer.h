#ifndef RT_RENDERER_H
#define RT_RENDERER_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <shader.h>
#include <camera.h>
#include "raytrace_camera.h"


#include <iostream>
#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <list>

#include "scene.h"




class RT_renderer {
  public:
	GLFWwindow * window = nullptr;
	Shader * shader = nullptr;
    Rect * screenCanvas = nullptr;
	raytrace_camera * RayTrace_camera = nullptr;
	Camera * GL_camera = nullptr;
	int image_width = 800;
	int image_height = 600;




	RT_renderer() {
		initialize();
	}

	void initialize() {
        //----------------------------------------------------------------------------------
        // glfw: initialize and configure
        // ------------------------------
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        //glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // Enable double buffering


        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif

        // glfw window creation
        // --------------------
        std::cout << "Creating GLFW window" << std::endl;
        window = glfwCreateWindow(image_width, image_height, "RTRT", NULL, NULL);

        // 将 this 指针设置为 GLFW 窗口的用户数据
        glfwSetWindowUserPointer(window, this);


        if (window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
        // set window position and size
        glfwMakeContextCurrent(window);

        glfwSwapInterval(_vSync ? 1 : 0); // Enable vsync

        // register callback functions
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);

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


        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glPointSize(8.0);
        glLineWidth(4.0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);




        // setup openGL camera
        GL_camera = new Camera(glm::vec3(5, 2, 3));

        


        


        // modify camera infos before render loop starts
        GL_camera->MovementSpeed = 1.0f;
        //camera.Front = glm::vec3(-0.373257, -0.393942, -0.826684);
        GL_camera->Front = glm::normalize(glm::vec3(0, 0, 0) - GL_camera->Position);


        // set up Dear ImGui context
         //imgui config----------------------
        ImGui_initialize();
        // --------------------------------

        Initialize_RayTrace_camera();

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

    void Initialize_RayTrace_camera() {
        // Setup ray_trace camera
        RayTrace_camera = new raytrace_camera();
        RayTrace_camera->aspect_ratio = static_cast<float>(image_width) / image_height;
        RayTrace_camera->image_width = image_width;
        RayTrace_camera->samples_per_pixel = 10;
        RayTrace_camera->max_depth = 8;
        RayTrace_camera->vfov = 20;
        RayTrace_camera->lookfrom = GL_camera->Position;
        RayTrace_camera->lookat = GL_camera->Front;
        RayTrace_camera->vup = GL_camera->WorldUp;
        RayTrace_camera->defocus_angle = 0; // no defocus blur, fuck it now, I hate it
        RayTrace_camera->focus_dist = 10;
    }



    void update_RayTrace_camera() {
		RayTrace_camera->lookfrom = GL_camera->Position;
		RayTrace_camera->lookat = GL_camera->Position + GL_camera->Front;
        RayTrace_camera->vfov = GL_camera->Zoom;
        RayTrace_camera->aspect_ratio = static_cast<float>(image_width) / image_height;


	}



    void process_input() {
        if (enable_keyboard_input) {
            processInput(window);
        }
	}

    void poll_events() {
        glfwPollEvents();
    }

    void swap_buffers() {
		glfwSwapBuffers(window);
	}   

	void render(std::vector<Object*> objects) {
		// render loop
		// -----------
        
        // per-frame time logic
        // --------------------
        per_frame_time_logic();

        // render part is here
        // ------
        // clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        RenderContext context;
        context.viewMatrix = GL_camera->GetViewMatrix();
        context.projectionMatrix = glm::perspective(glm::radians(GL_camera->Zoom), static_cast<float>(image_width) / static_cast<float>(image_height), 0.1f, 100.0f);
        // render the scene using openGL rasterization pipeline
        for (auto object : objects) {
            object->prepareDraw(context);
			object->draw();
		}


        if (screenCanvas != nullptr) {

            unsigned char* rendered_output = RayTrace_camera->rendered_image;
            screenCanvas->updateTexture(rendered_output, image_width, image_height,4);


            screenCanvas->draw();
        }




        // imgui---------------------------
        render_IMGUI();
        // --------------------------------





        
	}

    void reset_rendering() {
		// reset the rendered image by delete the screenCanvas
        if (screenCanvas != nullptr) {
			delete screenCanvas;
			screenCanvas = nullptr;
		}

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
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		// --------------------------------
	}   

    void ray_trace_render(const scene& Scene) {
        hittable_list RT_objects = Scene.RT_objects;
		RayTrace_camera->render_to_png(RT_objects);

        unsigned char* rendered_output = RayTrace_camera->rendered_image;
        if (screenCanvas == nullptr) {
            screenCanvas = new Rect();
		}
        screenCanvas->setShader(new Shader("../../shaders/texture_display.vs", "../../shaders/texture_display.fs"));
        screenCanvas->setTexture(rendered_output, image_width, image_height,4);

    }

    void ray_trace_render_thread(const scene& Scene) {
		RayTrace_camera->non_blocking_render(Scene.RT_objects, rendering_finished_flag);

        unsigned char* rendered_output = RayTrace_camera->rendered_image;
        if (screenCanvas == nullptr) {
            screenCanvas = new Rect();
        }
        screenCanvas->setShader(new Shader("../../shaders/texture_display.vs", "../../shaders/texture_display.fs"));
        screenCanvas->setTexture(rendered_output, image_width, image_height,4);
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
    bool is_mouse_input_enabled() {
		return enable_mouse_input;
	}
    bool is_keyboard_input_enabled() {
        return enable_keyboard_input;
    }
    bool is_camera_movement_enabled() {
		return enable_camera_movement;
	}

    bool has_RT_render_request_flag() {
		return RT_render_request_flag;
	}
    bool has_reset_request_flag() {
        return reset_request_flag;
    }
    bool has_rendering_finished_flag() {
		return rendering_finished_flag;
	}
    void reset_RT_render_request_flag() {
        RT_render_request_flag = false;
    }
    void reset_reset_request_flag() {
		reset_request_flag = false;
	}
    void reset_rendering_finished_flag() {
        rendering_finished_flag = false;
    }





    private:
    //----------------------------------------------------------------------------------
    const bool _vSync = true; // Enable vsync

    // some events flag
    bool RT_render_request_flag = false;
    bool reset_request_flag = false;
    bool rendering_finished_flag = false;

    // some control flags
    bool enable_mouse_input = true;
    bool enable_keyboard_input = true;
    bool enable_camera_movement = true;

    // input callback functions
    void adjust_window_size(int width, int height);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    void capture_mouse_movement(double xpos, double ypos);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    void capture_scroll_input(double xoffset, double yoffset);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void processInput(GLFWwindow* window);

    // mouse movement variables
    float lastX = image_width / 2.0f;
    float lastY = image_height / 2.0f;
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

};


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void RT_renderer::adjust_window_size(int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
void RT_renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // 获取类实例指针
    RT_renderer* renderer = static_cast<RT_renderer*>(glfwGetWindowUserPointer(window));

    // 调用实例的成员函数
    if (renderer) {
        renderer->adjust_window_size(width, height);
    }
}

void RT_renderer::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	// 获取类实例指针
	RT_renderer* renderer = static_cast<RT_renderer*>(glfwGetWindowUserPointer(window));

	// 调用实例的成员函数
    if (renderer) {
		renderer->capture_mouse_movement(xpos, ypos);
	}
}



// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void RT_renderer::capture_mouse_movement(double xposIn, double yposIn)
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
    }
    
}


void RT_renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// 获取类实例指针
	RT_renderer* renderer = static_cast<RT_renderer*>(glfwGetWindowUserPointer(window));

	// 调用实例的成员函数
    if (renderer) {
		renderer->capture_scroll_input(xoffset, yoffset);
	}
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void RT_renderer::capture_scroll_input(double xoffset, double yoffset)
{
    GL_camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void RT_renderer::processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (enable_camera_movement) {
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			GL_camera->ProcessKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			GL_camera->ProcessKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			GL_camera->ProcessKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			GL_camera->ProcessKeyboard(RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			GL_camera->ProcessKeyboard(UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			GL_camera->ProcessKeyboard(DOWN, deltaTime);
	}

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        RT_render_request_flag = true;
    }
    else {
        RT_render_request_flag = false;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        reset_request_flag = true;
    }
    else {
        reset_request_flag = false;
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








#endif