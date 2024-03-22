#ifndef SCENE_H
#define SCENE_H


#include "utils.h"

#include "raytrace_camera.h"
#include "color.h"
#include "hittable_list.h"
#include "bvh.h"
#include "material.h"
#include "sphere.h"
#include "triangle.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "object.h"
#include <shader.h>




#include <iostream>
#include <chrono>

class scene {
  public:
	hittable_list CPURT_objects; // ray tracing objects
    RT_Skybox CPURT_skybox; // ray tracing skybox

    std::vector<Object*> objects; // 存储场景中的物体 (not ray tracing, it is openGL rasterization objects)

    std::vector<Model*> rotate_models; // models that can be rotated by imgui
    
    // TODO: add lights
    //std::vector<Light*> lights;   // 存储场景中的光源
    scene(int initNum) : CPURT_skybox("../../resource/skybox") {// no ray tracing objects this case, only openGL rasterization objects
        // ------------------ openGL rasterization objects ------------------

        // skybox
        Skybox* skybox = new Skybox();
        skybox->setShader(new Shader("../../shaders/skybox_shader.vs", "../../shaders/skybox_shader.fs"));
        skybox->setTexture("../../resource/skybox");




        

        // the ground sphere
        Sphere* sphereObject2 = new Sphere();
        sphereObject2->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        sphereObject2->setColor(glm::vec4(0.5, 0.5, 0.5, 1.0));
        sphereObject2->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, -10, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10, 10, 10)));

        objects.push_back(sphereObject2);


        Model* teapot = new Model("../../resource/teapot.obj");
        teapot->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
        teapot->setShader(new Shader("../../shaders/mesh_shader.vs", "../../shaders/mesh_shader.fs"));
        objects.push_back(teapot);


        //Model* fish = new Model("../../resource/Amago0.obj");
        //fish->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(5.01, 5.01, 5.01)));
        //fish->setShader(new Shader("../../shaders/mesh_shader.vs", "../../shaders/mesh_shader.fs"));
        //objects.push_back(fish);


        rotate_models.push_back(teapot);
        //rotate_models.push_back(fish);

        
        // then render the skybox
        objects.push_back(skybox);

    }

	scene() : CPURT_skybox("../../resource/skybox") {
        // ------------------ ray tracing objects ------------------

        // different materials:
        // lambertian: diffuse material
        auto diffuse_material = make_shared<lambertian>(glm::vec3(0.5, 0.5, 0.5));
        CPURT_objects.add(make_shared<sphere>(glm::vec3(0, -10, 0), 10, diffuse_material));

        // dielectric: transparent material, has reflection and refraction
        auto glass_material = make_shared<dielectric>(1.5);
        CPURT_objects.add(make_shared<sphere>(glm::vec3(0, 1, -2.2), 1.0, glass_material));

        // diffuse material with texture
        shared_ptr<texture> earth_texture = make_shared<image_texture>("../../resource/earthmap.jpg");
        shared_ptr<material> earth_surface_material = make_shared<lambertian>(earth_texture);
        shared_ptr<sphere> earth = make_shared<sphere>(glm::vec3(0, 1, 2.2), 1.0, earth_surface_material);
        CPURT_objects.add(earth);
        //glm::quat q = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
        //earth->rotate(q);
        
        // metal: reflective material
        auto metal_material = make_shared<metal>(glm::vec3(0.7, 0.6, 0.5), 0.0);
        CPURT_objects.add(make_shared<sphere>(glm::vec3(0, 1, 0), 1.0, metal_material));

        // diffuse light material
        auto light_material = make_shared<diffuse_light>(glm::vec3(2, 2, 2));
        CPURT_objects.add(make_shared<sphere>(glm::vec3(1.5, 0.45, 0), 0.5, light_material));

        // triangle
        glm::vec3 v0(-1, 2, 0);
        glm::vec3 v1(1, 2, 0);
        glm::vec3 v2(0, 4, 0);
        CPURT_objects.add(make_shared<triangle>(v0, v1, v2, diffuse_material));
        //RT_objects.add(make_shared<triangle>(v0, v1, v2, earth_surface_material));



        // construct BVH
        hittable_list BVH_RT_objects;
        BVH_RT_objects = hittable_list(make_shared<BVH_node>(CPURT_objects));
        CPURT_objects = BVH_RT_objects;




        // ------------------ openGL rasterization objects ------------------

        // skybox
        Skybox * skybox = new Skybox();
        skybox->setShader(new Shader("../../shaders/skybox_shader.vs", "../../shaders/skybox_shader.fs"));
		skybox->setTexture("../../resource/skybox");




        // create openGL objects for the above ray tracing objects
        // the glass sphere
        Sphere * sphereObject = new Sphere();
        sphereObject->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        sphereObject->setColor(glm::vec4(0.3, 0.4, 0.8, 0.2));
        sphereObject->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, -2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        
        // the ground sphere
        Sphere * sphereObject2 = new Sphere();
        sphereObject2->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        sphereObject2->setColor(glm::vec4(0.5, 0.5, 0.5, 1.0));
        sphereObject2->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, -10, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10, 10, 10)));
        
        // the earth sphere
        Sphere* sphereObject3 = new Sphere();
        sphereObject3->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/texture_shader.fs"));
        sphereObject3->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0));
        sphereObject3->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        sphereObject3->setTexture("../../resource/earthmap.jpg");
        


        // the metal sphere
        Sphere* sphereObject4 = new Sphere();
        sphereObject4->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        sphereObject4->setColor(glm::vec4(0.7, 0.6, 0.5, 1.0));
        sphereObject4->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));

        //diffuse light sphere
        Sphere* sphereObject5 = new Sphere();
        sphereObject5->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        sphereObject5->setColor(glm::vec4(2, 2, 2, 1.0));
        sphereObject5->setModel(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.45, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));



        Triangle* triangleObject = new Triangle(v0, v1, v2);
        triangleObject->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        //triangleObject->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/texture_shader.fs"));
        triangleObject->setColor(glm::vec4(0.5, 0.5, 0.5, 1.0));
        triangleObject->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        //triangleObject->setTexture("../../resource/earthmap.jpg");
        
        objects.push_back(triangleObject);

        objects.push_back(sphereObject2);
        objects.push_back(sphereObject3);
        objects.push_back(sphereObject4);
        objects.push_back(sphereObject5);
        

        // then render the skybox
        objects.push_back(skybox);

        // put transparent objects at the end of the list, so that they are rendered last
        objects.push_back(sphereObject);


        
	}
};



#endif