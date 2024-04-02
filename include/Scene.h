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
    SkyboxTexture * skyboxTexture; // skybox texture
    std::vector<RayTraceObject*> rayTraceObjects; // temporary, for testing ray tracing objects
    // TODO: add lights
    //std::vector<Light*> lights; 
	Scene() : CPURT_skybox("../../resource/skybox") {
        // ------------------ ray tracing objects ------------------

        // different materials:
        // lambertian: diffuse material
        auto diffuse_material = make_shared<CPU_RAYTRACER::lambertian>(glm::vec3(0.5, 0.5, 0.5));
        //CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, -10, 0), 10, diffuse_material));
        auto ground = make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 0, 0), 1, diffuse_material);
        auto transformed_ground = make_shared<CPU_RAYTRACER::transform>(ground,glm::translate(glm::mat4(1.0), glm::vec3(0, -10, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10, 10, 10)));
        CPURT_objects.add(transformed_ground);


        // dielectric: transparent material, has reflection and refraction
        auto glass_material = make_shared<CPU_RAYTRACER::dielectric>(1.5);
        //CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 1, -2.2), 1.0, glass_material));
        auto glass = make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 0, 0), 1.0, glass_material);
        auto transformed_glass = make_shared<CPU_RAYTRACER::transform>(glass, glm::translate(glm::mat4(1.0), glm::vec3(0, 1, -2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        CPURT_objects.add(transformed_glass);

        // diffuse material with texture
        shared_ptr<CPU_RAYTRACER::texture> earth_texture = make_shared<CPU_RAYTRACER::image_texture>("../../resource/earthmap.jpg");
        shared_ptr<CPU_RAYTRACER::material> earth_surface_material = make_shared<CPU_RAYTRACER::lambertian>(earth_texture);
        //shared_ptr<CPU_RAYTRACER::sphere> earth = make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 1, 2.2), 1.0, earth_surface_material);
        auto earth = make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 0, 0), 1.0, earth_surface_material);
        glm::vec3 rotationAxis = glm::vec3(0, 1, 0); // 旋转轴（绕y轴）
        glm::mat4 identity = glm::mat4(1.0);
        auto transformed_earth = make_shared<CPU_RAYTRACER::transform>(earth,glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 2.2)) * glm::rotate(identity, glm::radians(90.0f), rotationAxis) *glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        CPURT_objects.add(transformed_earth);
        
        
        // metal: reflective material
        auto metal_material = make_shared<CPU_RAYTRACER::metal>(glm::vec3(0.7, 0.6, 0.5), 0.2);
        //CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 1, 0), 1.0, metal_material));
        auto metal = make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 0, 0), 1.0, metal_material);
        auto transformed_metal = make_shared<CPU_RAYTRACER::transform>(metal, glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        CPURT_objects.add(transformed_metal);

        // diffuse light material
        auto light_material = make_shared<CPU_RAYTRACER::diffuse_light>(glm::vec3(2, 2, 2));
        //CPURT_objects.add(make_shared<CPU_RAYTRACER::sphere>(glm::vec3(1.5, 0.45, 0), 0.5, light_material));
        auto light = make_shared<CPU_RAYTRACER::sphere>(glm::vec3(0, 0, 0), 1.0, light_material);
        auto transformed_light = make_shared<CPU_RAYTRACER::transform>(light, glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.45, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        CPURT_objects.add(transformed_light);

        // triangle
        glm::vec3 v0(-1, 2, -0.2);
        glm::vec3 v1(1, 2, 0.2);
        glm::vec3 v2(0, 4, 0);
        // CPURT_objects.add(make_shared<CPU_RAYTRACER::triangle>(v0, v1, v2, diffuse_material));
        auto triangle = make_shared<CPU_RAYTRACER::triangle>(v0, v1, v2, diffuse_material);
        auto transformed_triangle = make_shared<CPU_RAYTRACER::transform>(triangle, glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        CPURT_objects.add(transformed_triangle);




        // teapot
        auto diffuse_material2 = make_shared<CPU_RAYTRACER::lambertian>(glm::vec3(0.8, 0.6, 0.8));
        std::vector<shared_ptr<CPU_RAYTRACER::hittable>> teapot_triangles = CPU_RAYTRACER::load_mesh("../../resource/teapot.obj", diffuse_material2);
        auto cpu_teapot = make_shared<CPU_RAYTRACER::mesh>(teapot_triangles);
        auto transformed_teapot = make_shared<CPU_RAYTRACER::transform>(cpu_teapot, glm::translate(glm::mat4(1.0), glm::vec3(0, 2, 2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
        CPURT_objects.add(transformed_teapot);


        // diffuse material with texture
        shared_ptr<CPU_RAYTRACER::texture> night_texture = make_shared<CPU_RAYTRACER::image_texture>("../../resource/night.png");
        shared_ptr<CPU_RAYTRACER::material> cube_surface_material = make_shared<CPU_RAYTRACER::lambertian>(night_texture);
        std::vector<shared_ptr<CPU_RAYTRACER::hittable>> cube_meshes = CPU_RAYTRACER::load_mesh("../../resource/cube.obj", cube_surface_material);
        auto cube = make_shared<CPU_RAYTRACER::mesh>(cube_meshes);
        auto transformed_cube = make_shared<CPU_RAYTRACER::transform>(cube,glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.5, 2.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        CPURT_objects.add(transformed_cube);



        // construct BVH
        CPU_RAYTRACER::hittable_list BVH_RT_objects;
        BVH_RT_objects = CPU_RAYTRACER::hittable_list(make_shared<CPU_RAYTRACER::BVH_node>(CPURT_objects));
        CPURT_objects = BVH_RT_objects;




        // ------------------ openGL rasterization objects ------------------

        // skybox
        skyboxTexture = new SkyboxTexture();
        skyboxTexture->loadFromFolder("../../resource/skybox");
        skyboxTexture->createSkyboxTexture();
        Skybox * _skybox = new Skybox();
        _skybox->setShader(new Shader("../../shaders/skybox_shader.vert", "../../shaders/skybox_shader.frag"));
		    _skybox->setTexture(skyboxTexture);


        Model* teapot = new Model("../../resource/teapot.obj");
        RayTraceObject * rayTraceObject7 = new RayTraceObject(teapot);
        rayTraceObject7->setMaterial(LAMBERTIAN, 1.5, glm::vec4(0.8, 0.6, 0.8, 1.0));
        rayTraceObject7->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 2, 2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
        rayTraceObject7->update();
        rayTraceObjects.push_back(rayTraceObject7);
        objects.push_back(teapot);


        

        
        // the ground sphere
        Sphere * sphereObject2 = new Sphere();
        RayTraceObject * rayTraceObject2 = new RayTraceObject(sphereObject2);
        rayTraceObject2->setMaterial(LAMBERTIAN, 0.0, glm::vec4(0.5, 0.5, 0.5, 1.0));
        rayTraceObject2->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, -10, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10, 10, 10)));
        rayTraceObject2->update();
        rayTraceObjects.push_back(rayTraceObject2);
        objects.push_back(sphereObject2);

        
        // the earth sphere
        Texture * earthTexture = new Texture();
        earthTexture->loadFromFile("../../resource/earthmap.jpg");
        earthTexture->createGPUTexture();
        Sphere* sphereObject3 = new Sphere();
        RayTraceObject * rayTraceObject3 = new RayTraceObject(sphereObject3);
        rayTraceObject3->setMaterial(LAMBERTIAN, 0.0, glm::vec4(1.0, 1.0, 1.0, 1.0), earthTexture);
        
        rayTraceObject3->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 2.2)) * glm::rotate(identity, glm::radians(90.0f), rotationAxis) *glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject3->update();
        rayTraceObjects.push_back(rayTraceObject3);
        objects.push_back(sphereObject3);


        // the metal sphere
        Sphere* sphereObject4 = new Sphere();
        RayTraceObject * rayTraceObject4 = new RayTraceObject(sphereObject4);
        rayTraceObject4->setMaterial(METAL, 0.2, glm::vec4(0.7, 0.6, 0.5, 1.0));
        rayTraceObject4->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject4->update();
        rayTraceObjects.push_back(rayTraceObject4);
        objects.push_back(sphereObject4);

        // diffuse light sphere
        Sphere* sphereObject5 = new Sphere();
        RayTraceObject * rayTraceObject5 = new RayTraceObject(sphereObject5);
        rayTraceObject5->setMaterial(EMISSIVE, 0.0, glm::vec4(2, 2, 2, 1.0));
        rayTraceObject5->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.45, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        rayTraceObject5->update();
        rayTraceObjects.push_back(rayTraceObject5);
        objects.push_back(sphereObject5);



        Triangle* triangleObject = new Triangle(v0, v1, v2);
        RayTraceObject * rayTraceObject6 = new RayTraceObject(triangleObject);
        rayTraceObject6->setMaterial(LAMBERTIAN, 0.0, glm::vec4(0.5, 0.5, 0.5, 1.0));
        rayTraceObject6->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject6->update();
        rayTraceObjects.push_back(rayTraceObject6);
        objects.push_back(triangleObject);


        // the cube
        Texture * cubeTexture = new Texture();
        cubeTexture->loadFromFile("../../resource/night.png");
        cubeTexture->createGPUTexture();
        Model* cubeObject = new Model("../../resource/cube.obj");
        RayTraceObject * rayTraceObject8 = new RayTraceObject(cubeObject);
        rayTraceObject8->setMaterial(LAMBERTIAN, 0.0, glm::vec4(1.0, 1.0, 1.0, 1.0), cubeTexture);
        rayTraceObject8->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.5, 2.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        rayTraceObject8->update();
        rayTraceObjects.push_back(rayTraceObject8);
        objects.push_back(cubeObject);


        // then render the skybox
        objects.push_back(_skybox);

        // put transparent objects at the end of the list, so that they are rendered last
        // the glass sphere
        Sphere * sphereObject = new Sphere();
        RayTraceObject * rayTraceObject1 = new RayTraceObject(sphereObject);
        rayTraceObject1->setMaterial(DIELECTRIC, 1.5, glm::vec4(0.3, 0.4, 0.8, 0.2));
        rayTraceObject1->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, -2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject1->update();
        rayTraceObjects.push_back(rayTraceObject1);
        objects.push_back(sphereObject);


        
	}
};



#endif