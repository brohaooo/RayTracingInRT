#ifndef RAYTRACE_CONTROL_H
#define RAYTRACE_CONTROL_H

#include <Control.h>
#include <GPU_RAYTRACER/GPU_RAYTRACER.h>
#include "State.h"
#include <string>



class RayTraceCameraController : public CameraController {
public:
    RayTraceCameraController(Camera * camera, GPU_RAYTRACER::RaytraceManager * GPURT_manager) : CameraController(camera) {
        this->GPURT_manager = GPURT_manager;
    }

    void rotateCamera(double xposIn, double yposIn) override {
        CameraController::rotateCamera(xposIn, yposIn);
        if (enableCameraControl) {
            GPURT_manager->resetFrameCounter();
        }
    }

    void adjustfov(float yoffset) override {
        CameraController::adjustfov(yoffset);
        if (enableCameraControl) {
            GPURT_manager->resetFrameCounter();
        }
    }

private:
    GPU_RAYTRACER::RaytraceManager * GPURT_manager;
};







void keyboardActions(RTRTStateMachine * state_machine,RayTraceCameraController * camera_controller , float deltaTime, GLFWwindow* window, GPU_RAYTRACER::RaytraceManager * GPURT_manager
)
{    
    std::string current_state = state_machine->get_current_state()->name;
    if (current_state != "CPU ray-tracing") { // while in CPU ray tracing state, we disable the keyboard input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        if (camera_controller->canControlCamera()) {
            bool camera_moved = false;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
                camera_controller->moveCamera(deltaTime, FORWARD);
                camera_moved = true;
            }

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
                camera_controller->moveCamera(deltaTime, BACKWARD);
                camera_moved = true;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
                camera_controller->moveCamera(deltaTime, LEFT);
                camera_moved = true;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
                camera_controller->moveCamera(deltaTime, RIGHT);
                camera_moved = true;
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
                camera_controller->moveCamera(deltaTime, UP);
                camera_moved = true;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
                camera_controller->moveCamera(deltaTime, DOWN);
                camera_moved = true;
            }
            if (camera_moved) {
                if(GPURT_manager != nullptr) {
                    GPURT_manager->resetFrameCounter();
                }
            }
        }

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            camera_controller->setEnableCameraControl(false);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	    }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            camera_controller->setEnableCameraControl(true);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            state_machine->request_start_CPURT();
        }
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            state_machine->request_start_default_rendering();
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            state_machine->request_start_GPURT();
        }
    }
}









#endif