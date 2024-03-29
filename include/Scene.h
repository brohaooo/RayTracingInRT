#ifndef SCENE_H
#define SCENE_H


#include <CPU_RAYTRACER/CPU_RAYTRACER.h>
#include <GPU_RAYTRACER/GPU_RAYTRACER.h>


#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Object.h"
#include <Shader.h>




#include <iostream>
#include <chrono>

class Scene {
  public:
	CPU_RAYTRACER::hittable_list CPURT_objects; // ray tracing objects
    CPU_RAYTRACER::skybox CPURT_skybox; // ray tracing skybox

    std::vector<Object*> objects; // Scene objects to be rendered each frame (not ray tracing, it is openGL rasterization objects)
    std::vector<Model*> rotate_models; // models that can be rotated by imgui, tmp implementation, to rotate the teapot and fish from imgui
    
    // TODO: add lights
    //std::vector<Light*> lights; 
    Scene(int initNum) : CPURT_skybox("../../resource/skybox") {
        // ------------------ ray tracing objects ------------------
        // ground sphere in cpu ray tracing
        auto diffuse_material = make_shared<CPU_RAYTRACER::lambertian>(glm::vec3(0.5, 0.5, 0.5));
        auto glass_material = make_shared<CPU_RAYTRACER::dielectric>(1.5);
        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, -10, 0), 10, diffuse_material));
        auto light_material = make_shared<CPU_RAYTRACER::diffuse_light>(glm::vec3(4, 4, 4));
        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(2, 1, 0), 0.8, light_material));

        // a mesh teapot
        std::vector<shared_ptr<CPU_RAYTRACER::hittable>> mesh_vec = CPU_RAYTRACER::load_mesh("../../resource/teapot.obj", diffuse_material);
        CPU_RAYTRACER::mesh  teapot_mesh(mesh_vec, glass_material,glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
        CPURT_objects.add(make_shared<CPU_RAYTRACER::mesh>(teapot_mesh));
        // construct BVH
        CPU_RAYTRACER::hittable_list BVH_RT_objects;
        BVH_RT_objects = CPU_RAYTRACER::hittable_list(make_shared<CPU_RAYTRACER::BVH_node>(CPURT_objects));
        CPURT_objects = BVH_RT_objects;



        // ------------------ openGL rasterization objects ------------------

        // skybox
        Skybox* skybox = new Skybox();
        skybox->setShader(new Shader("../../shaders/skybox_shader.vs", "../../shaders/skybox_shader.fs"));
        skybox->setTexture("../../resource/skybox");


        // the ground sphere
        Sphere* sphereObject1 = new Sphere();
        sphereObject1->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        sphereObject1->setColor(glm::vec4(0.5, 0.5, 0.5, 1.0));
        sphereObject1->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, -10, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10, 10, 10)));

        objects.push_back(sphereObject1);

        // light sphere (for light source visualization)
        Sphere* sphereObject2 = new Sphere();
        sphereObject2->setShader(new Shader("../../shaders/texture_shader.vs", "../../shaders/shader.fs"));
        sphereObject2->setColor(glm::vec4(2, 2, 2, 1.0));
        sphereObject2->setModel(glm::translate(glm::mat4(1.0), glm::vec3(2, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.8, 0.8, 0.8)));

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

	Scene() : CPURT_skybox("../../resource/skybox") {
        // ------------------ ray tracing objects ------------------

        // different materials:
        // lambertian: diffuse material
        auto diffuse_material = make_shared<CPU_RAYTRACER::lambertian>(glm::vec3(0.5, 0.5, 0.5));
        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, -10, 0), 10, diffuse_material));

        // dielectric: transparent material, has reflection and refraction
        auto glass_material = make_shared<CPU_RAYTRACER::dielectric>(1.5);
        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 1, -2.2), 1.0, glass_material));

        // diffuse material with texture
        shared_ptr<CPU_RAYTRACER::texture> earth_texture = make_shared<CPU_RAYTRACER::image_texture>("../../resource/earthmap.jpg");
        shared_ptr<CPU_RAYTRACER::material> earth_surface_material = make_shared<CPU_RAYTRACER::lambertian>(earth_texture);
        shared_ptr<CPU_RAYTRACER::sphere> earth = make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 1, 2.2), 1.0, earth_surface_material);
        CPURT_objects.add(earth);
        //glm::quat q = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
        //earth->rotate(q);
        
        // metal: reflective material
        auto metal_material = make_shared<CPU_RAYTRACER::metal>(glm::vec3(0.7, 0.6, 0.5), 0.0);
        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 1, 0), 1.0, metal_material));

        // diffuse light material
        auto light_material = make_shared<CPU_RAYTRACER::diffuse_light>(glm::vec3(2, 2, 2));
        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(1.5, 0.45, 0), 0.5, light_material));

        // triangle
        glm::vec3 v0(-1, 2, 0);
        glm::vec3 v1(1, 2, 0);
        glm::vec3 v2(0, 4, 0);
        CPURT_objects.add(make_shared<CPU_RAYTRACER::triangle>(v0, v1, v2, diffuse_material));
        //CPURT_objects.add(make_shared<triangle>(v0, v1, v2, earth_surface_material));



        // construct BVH
        CPU_RAYTRACER::hittable_list BVH_RT_objects;
        BVH_RT_objects = CPU_RAYTRACER::hittable_list(make_shared<CPU_RAYTRACER::BVH_node>(CPURT_objects));
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