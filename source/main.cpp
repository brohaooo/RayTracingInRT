#include "Renderer.h"
#include "Scene.h"
#include "State.h"



int main() {
    //std::cout<< GPU_RAYTRACER::PrimitiveSize << std::endl;
    //std::cout<< GPU_RAYTRACER::BVHNodeSize << std::endl;
    //std::cout<<sizeof(glm::quat(1,0,0,0))<<std::endl;
    //return 0;
    
    
    Renderer renderer; // create renderer object, which contains all the rendering functions(glfw, imgui, etc.)
    InputHandler * inputHandler = renderer.inputHandler; // get input handler from renderer
    Scene Scene; // create Scene object, which contains all the objects in the Scene
    std::cout<<"scene created"<<std::endl;
    RTRTStateMachine state_machine; // create state machine object, which contains all the states and transitions, and handles the state changes
    state_machine.print_state(); // initial state is Default render state

    
    
    renderer.GPURT_manager->loadScene(Scene.objects);
    renderer.GPURT_manager->compute();
    renderer.screenCanvas->setShader(new Shader("../../shaders/texture_display.vert", "../../shaders/texture_display.frag"));
    renderer.GPURT_manager->setScreenCanvas(renderer.screenCanvas);



    // main loop
    while (!glfwWindowShouldClose(renderer.window))
    {



        
        // check last and current state
        std::string last_state = state_machine.get_current_state()->name;
        state_machine.update();
        std::string current_state = state_machine.get_current_state()->name;

        // if state changed, print it as debugging info
        if (last_state != current_state) {
			std::cout << "state changed from " << last_state << " to " << current_state << std::endl;
		}


        // things to do when state changes
        if (current_state == "Default render state") {
            if (last_state != "Default render state") {
                renderer.set_camera_movement(true);
                renderer.set_mouse_input(true);
            }

		}
        else if (current_state == "CPU ray-tracing") {
            if (last_state != "CPU ray-tracing") {
                // before starting ray tracing, render the scene using openGL rasterization pipeline (for a quick preview)
                // due to the double buffering, we need to render the scene twice to display the result (otherwise two frames will be recursively displayed)
                inputHandler->processKeyboardInput(); // update camera position and direction to newest values
                renderer.update_CPURT_camera(); // also update ray-tracing camera's position and direction to match the display camera's
                renderer.render(Scene, false);
                renderer.swap_buffers();
                renderer.render(Scene, false);
                // disable input and camera movement for ray tracing
                renderer.set_mouse_input(false);
                renderer.set_keyboard_input(false);
                renderer.set_camera_movement(false);    
                // start ray tracing thread
				renderer.CPURT_render_thread(Scene);
                // such thread will write the output image to an array, and then the main thread will copy the array to the texture
                // in renderer.render() function's updateTexture() call
			}
            renderer.updateCPUTexture(); // update the texture with the ray-tracing result (upload the array to the texture)

		}
        else if (current_state == "Displaying CPU ray-tracing result") {

            if (last_state == "CPU ray-tracing") {
				renderer.set_keyboard_input(true);
			}

		}
        else if (current_state == "GPU_ray_tracing state") {
            if (last_state != "GPU_ray_tracing state") {
                
                // disable input and camera movement for ray tracing
                renderer.set_mouse_input(true);
                renderer.set_keyboard_input(true);
                renderer.set_camera_movement(true);
                renderer.GPURT_manager->activate();
                renderer.GPURT_manager->resetFrameCounter();
            }
            // start ray tracing thread
            renderer.GPURT_manager->compute();
            
        }
        else {
			std::cout << "state not found" << std::endl;
		}

        // common tasks for all states
        inputHandler->processKeyboardInput(); // process input will be disabled in ray tracing state by disabling renderer's keyboard input
        // whether we should display the ray-tracing result on screen canvas or not
        bool display_screen_canvas = current_state == "CPU ray-tracing" || current_state == "Displaying CPU ray-tracing result" 
        || current_state == "GPU_ray_tracing state";
        renderer.render(Scene, display_screen_canvas); // render the scene using openGL rasterization pipeline


        // process state input by checking the renderer's flags
        if (renderer.has_RT_render_request_flag()) {
			state_machine.request_start_CPURT();
			renderer.reset_RT_render_request_flag();
		}
        if (renderer.has_reset_request_flag()) {
            state_machine.request_start_default_rendering();
            renderer.reset_reset_request_flag();
        }
        if (renderer.has_rendering_finished_flag()) {
			state_machine.request_display_CPURT();
			renderer.reset_rendering_finished_flag();
		}
        if (renderer.has_RT_switch_to_GPU_flag()) {
            state_machine.request_start_GPURT();
            renderer.reset_RT_switch_to_GPU_flag();
        }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        renderer.swap_buffers();
        renderer.poll_events();
	}


     
    return 0;
}


