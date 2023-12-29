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



int main() {

    
    scene Scene;
    RT_renderer renderer;

    //renderer.ray_trace_render(Scene);


    while (!glfwWindowShouldClose(renderer.window))
    {
        if (renderer.render_request) {
            //renderer.ray_trace_render(Scene);
            renderer.ray_trace_render_thread(Scene);
            renderer.render_request = false;
        }
        renderer.process_input();
        renderer.update_RayTrace_camera();
		renderer.render();
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        renderer.swap_buffers();
        renderer.poll_events();
	}


     
    return 0;
}


