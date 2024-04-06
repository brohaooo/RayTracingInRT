#ifndef CPU_RAYTRACER_RENDER_MANAGER_H
#define CPU_RAYTRACER_RENDER_MANAGER_H


#include "camera.h"
#include "../Camera.h"
#include "../Texture.h"
#include <vector>
#include "../Object.h"

namespace CPU_RAYTRACER {
    class render_manager
    {
    public:

        render_manager(int _width, int _height, const Camera * camera, Rect * _screenCanvas)
        {
            
            this->GL_camera = camera;
            // create cpu camera
            Initialize_CPURT_camera(_width, _height);
            this->screenCanvas = _screenCanvas;
            // set up the CPU ray tracing camera's output texture
            // it has an alpha channel, to blend with the openGL scene objects
            CPU_rendered_texture = new Texture(GL_UNSIGNED_BYTE, GL_RGBA);
            CPU_rendered_texture->setSize(_width, _height, 4);
            CPU_rendered_texture->createGPUTexture();


        }

        ~render_manager()
        {
        }

        void CPURT_render_thread() {
		    CPURT_camera->non_blocking_render(BVH_root, skybox);
            unsigned char* rendered_output = CPURT_camera->rendered_image;
            if (screenCanvas == nullptr) {
                std::cout<<"ERROR: screenCanvas is nullptr"<<std::endl;
                return;
            }
            // no matter how many times this function executes, we always use the same texture
            // so we don't need to create a new texture every time
            // and GPU ray tracer will use another texture to display the result
            screenCanvas->setTexture(CPU_rendered_texture);// first declear this empty texture to be used by the screenCanvas
            // update the texture with the new data
            int screen_height = CPURT_camera->image_width / CPURT_camera->aspect_ratio;
            CPU_rendered_texture->loadFromData(CPURT_camera->image_width, screen_height, 4, rendered_output);
            CPU_rendered_texture->updateGPUTexture();
	    }

        void updateCPUTexture() {
            if (CPU_rendered_texture == nullptr) {
                std::cout << "ERROR: CPU_rendered_texture is nullptr" << std::endl;
                return;
            }
            unsigned char* rendered_output = CPURT_camera->rendered_image;
            screenCanvas->setTexture(CPU_rendered_texture);
            int screen_width = CPURT_camera->image_width;
            int screen_height = CPURT_camera->image_width / CPURT_camera->aspect_ratio;
            CPU_rendered_texture->loadFromData(screen_width, screen_height, 4, rendered_output);
            CPU_rendered_texture->updateGPUTexture();
            screenCanvas->shader->use();
            screenCanvas->shader->setBool("flipYCoord", true);
        }

        void update_CPURT_camera() {
		    CPURT_camera->lookfrom = GL_camera->Position;
		    CPURT_camera->lookat = GL_camera->Position + GL_camera->Front;
            CPURT_camera->vfov = GL_camera->Zoom;
	    }

        // when the window size changes
        void resize_camera(int _screen_width, int _screen_height){
            CPURT_camera->aspect_ratio = static_cast<float>(_screen_width) / _screen_height;
            CPURT_camera->image_width = _screen_width;
        }

        bool isFinished(){
            return CPURT_camera->isFinished();
        }

        void resetFinished(){
            CPURT_camera->resetFinished();
        }

        void loadScene(const hittable_list &_rayTraceObjects, const skybox * _skybox) {
            // first clear the old objects
            if (rayTraceObjectsList == nullptr){// lazy initialization
                rayTraceObjectsList = make_shared<hittable_list>();
            }
            else{
                rayTraceObjectsList->clear();
            }
            skybox = _skybox;
            for (shared_ptr<hittable> rayTraceObject : _rayTraceObjects.objects) {
                rayTraceObjectsList->add(rayTraceObject);
            }
            updateBVH();
        }

        void updateBVH() {
            BVH_root.clear();
            for(shared_ptr<hittable> rayTraceObject : rayTraceObjectsList->objects) {
                BVH_root.add(rayTraceObject);
            }
            BVH_root = hittable_list(make_shared<CPU_RAYTRACER::BVH_node>(BVH_root));
        }

    private:
        const Camera * GL_camera = nullptr; // camera for OpenGL rendering
        camera * CPURT_camera = nullptr; // camera for CPU ray tracing, should be updated based on the OpenGL camera
        Rect * screenCanvas = nullptr; // screen canvas for OpenGL rendering
        Texture * CPU_rendered_texture = nullptr; // texture for CPU ray tracing

        shared_ptr<hittable_list> rayTraceObjectsList = nullptr; // it should be a list of hittable objects for CPU ray tracing (not the BVH tree!)
        hittable_list BVH_root = nullptr; // BVH root for CPU ray tracing, calculated from CPURT_objects
        const skybox * skybox = nullptr; // skybox for CPU ray tracing

        void Initialize_CPURT_camera(int _screen_width, int _screen_height) {
            // Setup ray_trace camera
            CPURT_camera = new CPU_RAYTRACER::camera();
            CPURT_camera->aspect_ratio = static_cast<float>(_screen_width) / _screen_height;
            CPURT_camera->image_width = _screen_width;
            CPURT_camera->samples_per_pixel = 10;
            CPURT_camera->max_depth = 8;
            CPURT_camera->vfov = GL_camera->Zoom;
            CPURT_camera->lookfrom = GL_camera->Position;
            CPURT_camera->lookat = GL_camera->Front;
            CPURT_camera->vup = GL_camera->WorldUp;
            CPURT_camera->defocus_angle = 0; // no defocus blur, fuck it now, I hate it
            CPURT_camera->focus_dist = 10;
        }

        

    };


}







#endif