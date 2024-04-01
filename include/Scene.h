#ifndef SCENE_H
#define SCENE_H


#include <CPU_RAYTRACER/CPU_RAYTRACER.h>
#include <GPU_RAYTRACER/GPU_RAYTRACER.h>


#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "RayTraceObject.h"
#include <Shader.h>




#include <iostream>
#include <chrono>

class Scene {
  public:
	CPU_RAYTRACER::hittable_list CPURT_objects; // ray tracing objects
    CPU_RAYTRACER::skybox CPURT_skybox; // ray tracing skybox

    std::vector<Object*> objects; // Scene objects to be rendered each frame (not ray tracing, it is openGL rasterization objects)
    std::vector<Model*> rotate_models; // models that can be rotated by imgui, tmp implementation, to rotate the teapot and fish from imgui
    Skybox* skybox; // skybox object
    std::vector<RayTraceObject*> rayTraceObjects; // temporary, for testing ray tracing objects
    // TODO: add lights
    //std::vector<Light*> lights; 
//    Scene(int initNum) : CPURT_skybox("../../resource/skybox") {
//        // ------------------ ray tracing objects ------------------
//        // ground sphere in cpu ray tracing
//        auto diffuse_material = make_shared<CPU_RAYTRACER::lambertian>(glm::vec3(0.5, 0.5, 0.5));
//        auto glass_material = make_shared<CPU_RAYTRACER::dielectric>(1.5);
//        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, -10, 0), 10, diffuse_material));
//        auto light_material = make_shared<CPU_RAYTRACER::diffuse_light>(glm::vec3(4, 4, 4));
//        CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(2, 1, 0), 0.8, light_material));
//
//        // a mesh teapot
//        std::vector<shared_ptr<CPU_RAYTRACER::hittable>> mesh_vec = CPU_RAYTRACER::load_mesh("../../resource/teapot.obj", diffuse_material);
//        CPU_RAYTRACER::mesh  teapot_mesh(mesh_vec, glass_material,glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
//        CPURT_objects.add(make_shared<CPU_RAYTRACER::mesh>(teapot_mesh));
//        // construct BVH
//        CPU_RAYTRACER::hittable_list BVH_RT_objects;
//        BVH_RT_objects = CPU_RAYTRACER::hittable_list(make_shared<CPU_RAYTRACER::BVH_node>(CPURT_objects));
//        CPURT_objects = BVH_RT_objects;
//
//
//
//        // ------------------ openGL rasterization objects ------------------
//
//        // skybox
//        Skybox* skybox = new Skybox();
//        skybox->setShader(new Shader("../../shaders/skybox_shader.vert", "../../shaders/skybox_shader.frag"));
//        skybox->setTexture("../../resource/skybox");
//
//
//        // the ground sphere
//        Sphere* sphereObject1 = new Sphere();
//        sphereObject1->setShader(new Shader("../../shaders/texture_shader.vert", "../../shaders/shader.frag"));
//        sphereObject1->setColor(glm::vec4(0.5, 0.5, 0.5, 1.0));
//        sphereObject1->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, -10, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10, 10, 10)));
//
//        objects.push_back(sphereObject1);
//
//        // light sphere (for light source visualization)
//        Sphere* sphereObject2 = new Sphere();
//        sphereObject2->setShader(new Shader("../../shaders/texture_shader.vert", "../../shaders/shader.frag"));
//        sphereObject2->setColor(glm::vec4(2, 2, 2, 1.0));
//        sphereObject2->setModel(glm::translate(glm::mat4(1.0), glm::vec3(2, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.8, 0.8, 0.8)));
//
//        objects.push_back(sphereObject2);
//
//
//        Model* teapot = new Model("../../resource/teapot.obj");
//        teapot->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
//        teapot->setShader(new Shader("../../shaders/mesh_shader.vert", "../../shaders/mesh_shader.frag"));
//        objects.push_back(teapot);
//
//
//        //Model* fish = new Model("../../resource/Amago0.obj");
//        //fish->setModel(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(5.01, 5.01, 5.01)));
//        //fish->setShader(new Shader("../../shaders/mesh_shader.vert", "../../shaders/mesh_shader.frag"));
//        //objects.push_back(fish);
//
//
//        rotate_models.push_back(teapot);
//        //rotate_models.push_back(fish);
//
//        
//        // then render the skybox
//        objects.push_back(skybox);
//
//    }

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
        glm::vec3 v0(-1, 2, -0.2);
        glm::vec3 v1(1, 2, 0.2);
        glm::vec3 v2(0, 4, 0);
        CPURT_objects.add(make_shared<CPU_RAYTRACER::triangle>(v0, v1, v2, diffuse_material));
        //CPURT_objects.add(make_shared<triangle>(v0, v1, v2, earth_surface_material));



        // construct BVH
        CPU_RAYTRACER::hittable_list BVH_RT_objects;
        BVH_RT_objects = CPU_RAYTRACER::hittable_list(make_shared<CPU_RAYTRACER::BVH_node>(CPURT_objects));
        CPURT_objects = BVH_RT_objects;




        // ------------------ openGL rasterization objects ------------------

        // skybox
        Skybox * _skybox = new Skybox();
        _skybox->setShader(new Shader("../../shaders/skybox_shader.vert", "../../shaders/skybox_shader.frag"));
		    _skybox->setTexture("../../resource/skybox");

        this->skybox = _skybox;


        Model* teapot = new Model("../../resource/teapot.obj");
        RayTraceObject * rayTraceObject7 = new RayTraceObject(teapot);
        rayTraceObject7->setMaterial(LAMBERTIAN, 1.5, -1, glm::vec4(0.8, 0.6, 0.8, 1.0));
        rayTraceObject7->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 2, 2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
        rayTraceObject7->update();
        rayTraceObjects.push_back(rayTraceObject7);
        objects.push_back(teapot);


        

        
        // the ground sphere
        Sphere * sphereObject2 = new Sphere();
        RayTraceObject * rayTraceObject2 = new RayTraceObject(sphereObject2);
        rayTraceObject2->setMaterial(LAMBERTIAN, 0.0, -1, glm::vec4(0.5, 0.5, 0.5, 1.0));
        rayTraceObject2->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, -10, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10, 10, 10)));
        rayTraceObject2->update();
        rayTraceObjects.push_back(rayTraceObject2);
        objects.push_back(sphereObject2);

        
        // the earth sphere
        Sphere* sphereObject3 = new Sphere();
        RayTraceObject * rayTraceObject3 = new RayTraceObject(sphereObject3);
        rayTraceObject3->setMaterial(LAMBERTIAN, 0.0, 0, glm::vec4(1.0, 1.0, 1.0, 1.0), "../../resource/earthmap.jpg");
        rayTraceObject3->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject3->update();
        rayTraceObjects.push_back(rayTraceObject3);
        objects.push_back(sphereObject3);


        // the metal sphere
        Sphere* sphereObject4 = new Sphere();
        RayTraceObject * rayTraceObject4 = new RayTraceObject(sphereObject4);
        rayTraceObject4->setMaterial(METAL, 0.0, -1, glm::vec4(0.7, 0.6, 0.5, 1.0));
        rayTraceObject4->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject4->update();
        rayTraceObjects.push_back(rayTraceObject4);
        objects.push_back(sphereObject4);

        //diffuse light sphere
        Sphere* sphereObject5 = new Sphere();
        RayTraceObject * rayTraceObject5 = new RayTraceObject(sphereObject5);
        rayTraceObject5->setMaterial(LAMBERTIAN, 0.0, -1, glm::vec4(2, 2, 2, 1.0));
        rayTraceObject5->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.45, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        rayTraceObject5->update();
        rayTraceObjects.push_back(rayTraceObject5);
        objects.push_back(sphereObject5);



        Triangle* triangleObject = new Triangle(v0, v1, v2);
        RayTraceObject * rayTraceObject6 = new RayTraceObject(triangleObject);
        rayTraceObject6->setMaterial(LAMBERTIAN, 0.0, -1, glm::vec4(0.5, 0.5, 0.5, 1.0));
        rayTraceObject6->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject6->update();
        rayTraceObjects.push_back(rayTraceObject6);
        objects.push_back(triangleObject);


        

        

        
        
        
        

        // then render the skybox
        objects.push_back(skybox);

        // put transparent objects at the end of the list, so that they are rendered last
        // the glass sphere
        Sphere * sphereObject = new Sphere();
        RayTraceObject * rayTraceObject1 = new RayTraceObject(sphereObject);
        rayTraceObject1->setMaterial(DIELECTRIC, 1.5, -1, glm::vec4(0.3, 0.4, 0.8, 0.2));
        rayTraceObject1->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, -2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject1->update();
        rayTraceObjects.push_back(rayTraceObject1);
        objects.push_back(sphereObject);


        
	}
};



#endif