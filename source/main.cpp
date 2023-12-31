#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "utils.h"

#include "raytrace_camera.h"
#include "color.h"
#include "hittable_list.h"
#include "bvh.h"
#include "material.h"
#include "sphere.h"

#include <shader.h>
#include <camera.h>

#include <iostream>
#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <list>

#include "RT_renderer.h"

#include "state.h"



int main() {

    
    
    RT_renderer renderer; // create renderer object, which contains all the rendering functions(glfw, imgui, etc.)
    scene Scene; // create scene object, which contains all the objects in the scene
    std::cout<<"scene created"<<std::endl;
    RTRTStateMachine state_machine; // create state machine object, which contains all the states and transitions, and handles the state changes
    state_machine.print_state(); // initial state is idle

    // TODO: extract event handling to a separate class from renderer


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

        if (current_state == "idle") {
            if (last_state == "displaying") {
            	renderer.reset_rendering();
                renderer.set_camera_movement(true);
                renderer.set_mouse_input(true);
            }

		}
        else if (current_state == "ray_tracing") {
            if (last_state == "idle") {
                renderer.set_mouse_input(false);
                renderer.set_keyboard_input(false);
                renderer.set_camera_movement(false);
				renderer.ray_trace_render_thread(Scene);// if state changed from idle to ray tracing, start ray tracing thread
                // such thread will write the output image to an array, and then the main thread will copy the array to the texture
                // in renderer.render() function's updateTexture() call
			}

		}
        else if (current_state == "displaying") {

            if (last_state == "ray_tracing") {
				renderer.set_keyboard_input(true);
			}

		}
        else {
			std::cout << "state not found" << std::endl;
		}

        // common tasks for all states
        renderer.process_input(); // process input will be disabled in ray tracing state by disabling renderer's keyboard input
        renderer.update_RayTrace_camera(); // update ray-tracing camera's position and direction to match the display camera's
        renderer.render(Scene.objects); // render the scene using openGL rasterization pipeline



        // process state input by checking the renderer's flags
        if (renderer.has_RT_render_request_flag()) {
			state_machine.set_input("start_RT");
			renderer.reset_RT_render_request_flag();
		}
        if (renderer.has_reset_request_flag()) {
            state_machine.set_input("stop_display");
            renderer.reset_reset_request_flag();
        }
        if (renderer.has_rendering_finished_flag()) {
			state_machine.set_input("finish_RT");
			renderer.reset_rendering_finished_flag();
		}


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        renderer.swap_buffers();
        renderer.poll_events();
	}


     
    return 0;
}


