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


struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    Light(glm::vec3 _position, glm::vec3 _color, float _intensity) : position(_position), color(_color), intensity(_intensity) {}
    void set(glm::vec3 _position, glm::vec3 _color, float _intensity) {// update the light
        position = _position;
        color = _color;
        intensity = _intensity;
    }
};

class Scene {
public:
    std::vector<SceneObject*> sceneObjects;
    std::vector<std::shared_ptr<RenderComponent>> renderQueue; // object components to be rendered each frame

    void sortRenderQueue() {
        std::sort(renderQueue.begin(), renderQueue.end(), [](std::shared_ptr<RenderComponent> a, std::shared_ptr<RenderComponent> b) {
            return a->renderPriority < b->renderPriority;
        });
    }

    Scene() {
    }

    virtual ~Scene() {
        for (SceneObject* sceneObject : sceneObjects) {
            delete sceneObject;
        }
    }
};


class RayTraceScene : public Scene {
public:
    
    CPU_RAYTRACER::hittable_list CPURT_objects; // ray tracing objects
    CPU_RAYTRACER::skybox CPURT_skybox; // ray tracing skybox
    SkyboxTexture * skyboxTexture; // skybox texture
    // rayTraceObjects: A list for tracking all ray tracing-related objects. 
    // Each instance of RayTraceObject simultaneously encapsulates the necessary data and functionality for three distinct rendering techniques: CPU ray tracing, GPU ray tracing, and rasterization. 
    // This design allows each RayTraceObject to be easily added and updated in these three rendering pipelines. 
    std::vector<RayTraceObject*> rayTraceObjects; 

    RayTraceScene(){
    
        glm::vec3 rotationAxis = glm::vec3(0, 1, 0);
        glm::mat4 identity = glm::mat4(1.0);
        glm::vec3 v0(-1, 2, -0.2);
        glm::vec3 v1(1, 2, 0.2);
        glm::vec3 v2(0, 4, 0);
        
        
        // skybox
        skyboxTexture = new SkyboxTexture();
        skyboxTexture->loadFromFolder("resource/skybox");
        skyboxTexture->createSkyboxTexture();
        GSkybox * _skybox = new GSkybox();
        _skybox->setShader(new Shader("shaders/skybox_shader.vert", "shaders/skybox_shader.frag"));
        _skybox->setTexture(skyboxTexture);
        RayTraceSkybox * rayTraceSkybox = new RayTraceSkybox(_skybox);
        CPURT_skybox = *rayTraceSkybox->CPU_skybox;
        rayTraceSkybox->attachToSceneRenderList(renderQueue);
        
        
        // teapot, would be a performance bottleneck because of the high triangle count
        GModel* teapot = new GModel("resource/teapot.obj");
        teapot->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject7 = new RayTraceObject(teapot);
        rayTraceObject7->setMaterial(LAMBERTIAN, 1.5, glm::vec4(0.8, 0.6, 0.8, 1.0));
        rayTraceObject7->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 2, 2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01, 0.01, 0.01)));
        rayTraceObject7->update();
        rayTraceObjects.push_back(rayTraceObject7);
        rayTraceObject7->attachToSceneRenderList(renderQueue);
        
        
        
        // the ground sphere
        GSphere * sphereObject2 = new GSphere();
        sphereObject2->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject2 = new RayTraceObject(sphereObject2);
        rayTraceObject2->setMaterial(LAMBERTIAN, 0.0, glm::vec4(0.5, 0.5, 0.5, 1.0));
        rayTraceObject2->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, -100, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(100, 100, 100)));
        rayTraceObject2->update();
        rayTraceObjects.push_back(rayTraceObject2);
        rayTraceObject2->attachToSceneRenderList(renderQueue);
        
        
        // the earth sphere
        Texture * earthTexture = new Texture();
        earthTexture->loadFromFile("resource/earthmap.jpg");
        earthTexture->createGPUTexture();
        GSphere* sphereObject3 = new GSphere();
        sphereObject3->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject3 = new RayTraceObject(sphereObject3);
        rayTraceObject3->setMaterial(LAMBERTIAN, 0.0, glm::vec4(1.0, 1.0, 1.0, 1.0), earthTexture);
        
        rayTraceObject3->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 2.2)) * glm::rotate(identity, glm::radians(90.0f), rotationAxis) *glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject3->update();
        rayTraceObjects.push_back(rayTraceObject3);
        rayTraceObject3->attachToSceneRenderList(renderQueue);
        
        
        // the metal sphere
        GSphere* sphereObject4 = new GSphere();
        sphereObject4->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject4 = new RayTraceObject(sphereObject4);
        rayTraceObject4->setMaterial(METAL, 0.2, glm::vec4(0.7, 0.6, 0.5, 1.0));
        rayTraceObject4->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject4->update();
        rayTraceObjects.push_back(rayTraceObject4);
        rayTraceObject4->attachToSceneRenderList(renderQueue);
        
        // diffuse light sphere
        GSphere* sphereObject5 = new GSphere();
        sphereObject5->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject5 = new RayTraceObject(sphereObject5);
        rayTraceObject5->setMaterial(EMISSIVE, 0.0, glm::vec4(2, 2, 2, 1.0));
        rayTraceObject5->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.45, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        rayTraceObject5->update();
        rayTraceObjects.push_back(rayTraceObject5);
        rayTraceObject5->attachToSceneRenderList(renderQueue);
        
        
        
        GTriangle* triangleObject = new GTriangle(v0, v1, v2);
        triangleObject->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject6 = new RayTraceObject(triangleObject);
        rayTraceObject6->setMaterial(LAMBERTIAN, 0.0, glm::vec4(0.5, 0.5, 0.5, 1.0));
        rayTraceObject6->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject6->update();
        rayTraceObjects.push_back(rayTraceObject6);
        rayTraceObject6->attachToSceneRenderList(renderQueue);
        
        
        // the cube
        Texture * cubeTexture = new Texture();
        cubeTexture->loadFromFile("resource/night.png");
        cubeTexture->createGPUTexture();
        GModel* cubeObject = new GModel("resource/cube.obj");
        cubeObject->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject8 = new RayTraceObject(cubeObject);
        rayTraceObject8->setMaterial(LAMBERTIAN, 0.0, glm::vec4(1.0, 1.0, 1.0, 1.0), cubeTexture);
        rayTraceObject8->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.5, 2.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        rayTraceObject8->update();
        rayTraceObjects.push_back(rayTraceObject8);
        rayTraceObject8->attachToSceneRenderList(renderQueue);
        
        // the glass sphere (transparent object in rendering order)
        GSphere * sphereObject = new GSphere();
        sphereObject->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject1 = new RayTraceObject(sphereObject,TRANSPARENT);
        rayTraceObject1->setMaterial(DIELECTRIC, 1.5, glm::vec4(0.3, 0.4, 0.8, 0.6));
        rayTraceObject1->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, -2.2)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject1->update();
        rayTraceObjects.push_back(rayTraceObject1);
        rayTraceObject1->attachToSceneRenderList(renderQueue);
        
        
        
        // sort the renderQueue based on renderPriority
        // the lower the renderPriority, the earlier it is rendered
        std::sort(renderQueue.begin(), renderQueue.end(), [](std::shared_ptr<RenderComponent> a, std::shared_ptr<RenderComponent> b) {
            return a->renderPriority < b->renderPriority;
        });


        // remember to add all these RayTraceObjects to sceneObjects
        for(RayTraceObject* rayTraceObject : rayTraceObjects) {
            sceneObjects.push_back(rayTraceObject);
        }
    
    
        // ------------------ temporary, now use a hittable_list to store all objects which can be parsed to the CPU ray tracer ------------------
        for(RayTraceObject* rayTraceObject : rayTraceObjects) {
                CPURT_objects.add(rayTraceObject->CPU_object);
        }
            
    }

};



#endif