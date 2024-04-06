#ifndef CONTROL_H
#define CONTROL_H

#include <Camera.h>


// we need a class to have a callback function for mouse movement to update the camera
class CameraController {
public:
    CameraController(Camera* camera)
        : camera(camera), lastX(0.0f), lastY(0.0f), firstMouse(true) {}

    virtual void rotateCamera(double xposIn, double yposIn) {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;  // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;
        if (enableCameraControl) {
            camera->ProcessMouseMovement(xoffset, yoffset);
        }
    }

    virtual void adjustfov(float yoffset) {
        if (enableCameraControl){
            camera->ProcessMouseScroll(static_cast<float>(yoffset));
        }
    }

    // just warp the camera's processKeyboard function for better hierarchy and readability
    virtual void moveCamera(float deltaTime, enum Camera_Movement direction) {
        if (enableCameraControl) {
            camera->ProcessKeyboard(direction, deltaTime);
        }
    }


    void setEnableCameraControl(bool enable) {
        enableCameraControl = enable;
    }

    bool canControlCamera() {
        return enableCameraControl;
    }

protected:
    Camera* camera;
    float lastX, lastY;
    bool firstMouse;
    bool enableCameraControl = true;
};











#endif
