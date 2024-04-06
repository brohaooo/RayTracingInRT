#include "Renderer.h"
#include "Scene.h"
#include "State.h"
#include "RayTraceControl.h"
#include "FrameRateMonitor.h"
#include "InputHandler.h"




int main() {
    int initial_width = 800;
    int initial_height = 600;

    // --------------------------------- create camera object ----------------------------------
    Camera * camera = new Camera(glm::vec3(0, 2, 5));
    // modify camera infos before render loop starts
    camera->MovementSpeed = 1.0f;
    camera->Front = glm::normalize(glm::vec3(0, 2, 0) - camera->Position);
    camera->ProcessMouseMovement(0, 0); // to update the right and up vector
    // -----------------------------------------------------------------------------------------
    // ---- create renderer, which is used for rendering ---------------------------------------
    Renderer renderer(initial_width, initial_height, camera);
    // --- get screen canvas from renderer, it will be the final output in deferred rendering --
    // it will be used to display the ray tracing result
    GRect * screenCanvas = renderer.screenCanvas; 
    screenCanvas->setShader(new Shader("shaders/texture_display.vert", "shaders/texture_display.frag"));
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create Scene object --------------------------------------
    RayTraceScene Scene; // create Scene object, which contains all the objects in the Scene
    std::cout<<"scene created"<<std::endl;
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create GPU ray tracer manager object ---------------------
    GPU_RAYTRACER::RaytraceManager * GPURT_manager = new GPU_RAYTRACER::RaytraceManager(initial_width, initial_height, camera, screenCanvas);
    // attach the GPU ray tracer manager to the renderer so that it can be used in the rendering loop
    // currently, the manager can generate BVH visualization commnad to renderer
    renderer.GPURT_manager = GPURT_manager; 
    // ------------------------ load the scene to the GPU ray tracer ---------------------------
    GPURT_manager->loadScene(Scene.rayTraceObjects, Scene.skyboxTexture); // stored in the GPU ray tracer manager
    GPURT_manager->setScreenCanvas(screenCanvas);
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create CPU ray tracer manager object ---------------------
    CPU_RAYTRACER::render_manager * CPURT_manager = new CPU_RAYTRACER::render_manager(initial_width, initial_height, camera, screenCanvas);
    // currently, renderer doesn't use the CPU ray tracer manager, so we don't attach it to the renderer
    // but the manager can visualize its ray tracing process and result by modifying the screen canvas
    // ------------------------ load the scene to the CPU ray tracer ---------------------------
    CPURT_manager->loadScene(Scene.CPURT_objects, &Scene.CPURT_skybox);
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create camera controller object --------------------------
    // it is used to control the camera movement and rotation
    // it also update the GPU ray tracer manager's camera (CPU one is updated when new CPU ray tracing starts)
    RayTraceCameraController * cameraController = new RayTraceCameraController(camera, GPURT_manager); // create camera controller object, which contains the camera and the ray tracer manager
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create State machine object ------------------------------
    RTRTStateMachine state_machine; // create state machine object, which contains all the states and transitions, and handles the state changes
    state_machine.print_state(); // initial state is Default render state
    // -----------------------------------------------------------------------------------------
    // ------------------------------ create Frame rate monitor object -------------------------
    FrameRateMonitor * frameRateMonitor = new FrameRateMonitor();
    renderer.frameRateMonitor = frameRateMonitor; // attach the frame rate monitor to the renderer for displaying fps
    // -------------- register user input callbacks and loop check functions -------------------
    // create input handler object, which contains the input handling functions
    InputHandler * inputHandler = new InputHandler(renderer.window); 
    // set the input handling functions via callbacks and lambda functions
    inputHandler->setMouseMovementCallback([cameraController](double xpos, double ypos) {
        cameraController->rotateCamera(xpos, ypos);
    });
    inputHandler->setScrollCallback([cameraController](double yoffset) {
        cameraController->adjustfov(yoffset);
    });
    inputHandler->setFramebufferSizeCallback([&renderer,GPURT_manager,CPURT_manager](int width, int height) {
        renderer.resize(width, height);
        // update the ray tracing cameras
        // CPU
        if (CPURT_manager != nullptr) {
            CPURT_manager->resize_camera(width, height);
        }
        // GPU
        if (GPURT_manager != nullptr) {
            // reset the frame count for GPU ray tracing
            GPURT_manager->resetFrameCounter();
            GPURT_manager->changeScreenSize(width, height);
        };
    });
    // set the keyboard action execution function in while loop update (it will be called in the main loop other than callbacks)
    inputHandler->setKeyboardActionExecution([&renderer, &state_machine, cameraController, GPURT_manager,frameRateMonitor]() {
        float deltaTime = frameRateMonitor->getFrameDeltaTime();
        GLFWwindow * window = renderer.window;
        keyboardActions(&state_machine, cameraController, deltaTime, window, GPURT_manager);
    });
    // -----------------------------------------------------------------------------------------
    

    // main loop
    while (!glfwWindowShouldClose(renderer.window))
    {
        // -------------------------- logic part of the main loop --------------------------------
        // update the frame rate monitor
        frameRateMonitor->update();

        // update the state machine
        std::string last_state = state_machine.get_current_state()->name;
        state_machine.update();
        std::string current_state = state_machine.get_current_state()->name;
        // process keyboard input (checking all the keys every frame)
        inputHandler->processKeyboardInput(); // process input will be disabled in ray tracing state by disabling renderer's keyboard input



        // update model matrices for scene objects
        if (current_state == "Default render state" || current_state == "GPU_ray_tracing state"){
            for (RayTraceObject * rayTraceObject : Scene.rayTraceObjects) {
                rayTraceObject->setModelMatrix(glm::rotate(rayTraceObject->getModelMatrix(),glm::radians(10.0f) * frameRateMonitor->getFrameDeltaTime(), glm::vec3(0, 1, 0)));
            }
            GPURT_manager->updateTLAS();
        }


        // if state changed, print it as debugging info
        if (last_state != current_state) {
			std::cout << "state changed from " << last_state << " to " << current_state << std::endl;
		}

        
        // things to do when state changes
        if (current_state == "Default render state") {
            if (last_state != "Default render state") {
                if (last_state == "Displaying CPU ray-tracing result") {
                    cameraController->setEnableCameraControl(true);
                }     
            }
		}
        else if (current_state == "CPU ray-tracing") {
            if (last_state != "CPU ray-tracing") {
                // before starting ray tracing, render the scene using openGL rasterization pipeline (for a quick preview)
                // due to the double buffering, we need to render the scene twice to display the result (otherwise two frames will be recursively displayed)
                CPURT_manager->update_CPURT_camera();
                renderer.render(Scene, false);
                renderer.swap_buffers();
                renderer.render(Scene, false);
                // disable input and camera movement for ray tracing
                cameraController->setEnableCameraControl(false);
                // start ray tracing thread
                // such thread will write the output image to an array, and then the main thread will copy the array to the texture
                CPURT_manager->CPURT_render_thread();

			}
            else{
                // check if the ray tracing thread has finished
                if (CPURT_manager->isFinished()) {
                    CPURT_manager->resetFinished();
                    state_machine.request_display_CPURT();
                }

            }
            // renderer.updateCPUTexture(); // update the texture with the ray-tracing result (upload the data color array to the texture)
            CPURT_manager->updateCPUTexture();

		}
        else if (current_state == "Displaying CPU ray-tracing result") {
            // do nothing, just display the ray-tracing result on screen canvas
		}
        else if (current_state == "GPU_ray_tracing state") {
            if (last_state != "GPU_ray_tracing state") {
                
                if (last_state == "Displaying CPU ray-tracing result") {
                    
                    cameraController->setEnableCameraControl(true);
                }                
                renderer.GPURT_manager->activate();
                renderer.GPURT_manager->resetFrameCounter();
            }
            // start ray tracing thread
            renderer.GPURT_manager->compute();
            
        }
        else {
			std::cout << "state not found" << std::endl;
		}


        // -----------------------------------------------------------------------------------------

        // -------------------------- rendering part of the main loop ------------------------------

        // common tasks for all states
        // whether we should display the ray-tracing result on screen canvas or not
        bool display_screen_canvas = current_state == "CPU ray-tracing" || current_state == "Displaying CPU ray-tracing result" 
        || current_state == "GPU_ray_tracing state";
        bool draw_imgui = current_state == "Default render state" || current_state == "GPU_ray_tracing state";
        renderer.render(Scene, display_screen_canvas,draw_imgui); // render the scene using openGL rasterization pipeline

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        renderer.swap_buffers();
        renderer.poll_events();
        
        // -----------------------------------------------------------------------------------------

	}


     
    return 0;
}


