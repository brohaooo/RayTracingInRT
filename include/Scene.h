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
    std::vector<SceneObject*> sceneObjects;
    std::vector<std::shared_ptr<RenderComponent>> renderQueue; // object components to be rendered each frame
    std::vector<Light*> sceneLights; // point lights in the scene

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
    RenderContext * renderContext = nullptr; // render context for containing light sources and other rendering-related data, owned by renderer, here just for reference's need

    RayTraceScene(RenderContext* _renderContext = nullptr) {
        renderContext = _renderContext;
    
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
        
        
        // waifu, would be a performance bottleneck because of the high triangle count
        Texture * waifuTexture = new Texture();
        waifuTexture->loadFromFile("resource/mebius_diffuse.png");
        waifuTexture->removeAlphaChannel();
        waifuTexture->resizeData(1024, 1024,3);
        waifuTexture->createGPUTexture();
        GModel* waifu = new GModel("resource/mebius.obj");
        waifu->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject7 = new RayTraceObject(waifu,OPAQUE,renderContext);
        rayTraceObject7->setMaterial(LAMBERTIAN, 1.5, glm::vec4(1.0,1.0,1.0, 1.0), waifuTexture);
        rayTraceObject7->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 2, 2.2)) * glm::rotate(identity, glm::radians(-95.0f), rotationAxis) * glm::scale(glm::mat4(1.0), glm::vec3(0.08, 0.08, 0.08)));
        rayTraceObject7->update();
        rayTraceObjects.push_back(rayTraceObject7);
        rayTraceObject7->attachToSceneRenderList(renderQueue);
        
        
        
        // the ground sphere
        GSphere * sphereObject2 = new GSphere();
        sphereObject2->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject2 = new RayTraceObject(sphereObject2,OPAQUE,renderContext);
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
        RayTraceObject * rayTraceObject3 = new RayTraceObject(sphereObject3,OPAQUE,renderContext);
        rayTraceObject3->setMaterial(LAMBERTIAN, 0.0, glm::vec4(1.0, 1.0, 1.0, 1.0), earthTexture);
        
        rayTraceObject3->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 2.2)) * glm::rotate(identity, glm::radians(90.0f), rotationAxis) *glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject3->update();
        rayTraceObjects.push_back(rayTraceObject3);
        rayTraceObject3->attachToSceneRenderList(renderQueue);
        
        
        // the metal sphere
        GSphere* sphereObject4 = new GSphere();
        sphereObject4->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject4 = new RayTraceObject(sphereObject4,OPAQUE,renderContext);
        rayTraceObject4->setMaterial(METAL, 0.2, glm::vec4(0.7, 0.6, 0.5, 1.0));
        rayTraceObject4->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1, 1, 1)));
        rayTraceObject4->update();
        rayTraceObjects.push_back(rayTraceObject4);
        rayTraceObject4->attachToSceneRenderList(renderQueue);
        
        // diffuse light sphere
        GSphere* sphereObject5 = new GSphere();
        sphereObject5->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject5 = new RayTraceObject(sphereObject5,OPAQUE,renderContext);
        rayTraceObject5->setMaterial(EMISSIVE, 0.0, glm::vec4(2, 2, 2, 1.0));
        rayTraceObject5->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.45, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        rayTraceObject5->update();
        rayTraceObjects.push_back(rayTraceObject5);
        rayTraceObject5->attachToSceneRenderList(renderQueue);
        // record the light source to sceneObjects
        PointLight * sphereObject5Light = new PointLight(glm::vec3(1.5, 0.45, 0), glm::vec3(2, 2, 2));
        sceneObjects.push_back(sphereObject5Light); // then such light can be updated in the logic loop by ticking its component
        sceneLights.push_back(sphereObject5Light);
        // add periodic translation component to the light source mesh
        std::unique_ptr<ObjectPeriodicTranslationComponent> sphereObject5TranslationComponent = std::make_unique<ObjectPeriodicTranslationComponent>(glm::vec3(0, 0, 1), glm::vec3(0, 0, 1));
        rayTraceObject5->addComponent(std::move(sphereObject5TranslationComponent));
        // we also update the light source Light object in the logic loop by ticking its component
        std::unique_ptr<ObjectPeriodicTranslationComponent> sphereObject5TranslationComponent2 = std::make_unique<ObjectPeriodicTranslationComponent>(glm::vec3(0, 0, 1), glm::vec3(0, 0, 1));
        sphereObject5Light->addComponent(std::move(sphereObject5TranslationComponent2));
        
        
        GTriangle* triangleObject = new GTriangle(v0, v1, v2);
        triangleObject->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject6 = new RayTraceObject(triangleObject,OPAQUE,renderContext);
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
        RayTraceObject * rayTraceObject8 = new RayTraceObject(cubeObject,OPAQUE,renderContext);
        rayTraceObject8->setMaterial(LAMBERTIAN, 0.0, glm::vec4(1.0, 1.0, 1.0, 1.0), cubeTexture);
        rayTraceObject8->setModelMatrix(glm::translate(glm::mat4(1.0), glm::vec3(1.5, 0.5, 2.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5, 0.5, 0.5)));
        rayTraceObject8->update();
        rayTraceObjects.push_back(rayTraceObject8);
        rayTraceObject8->attachToSceneRenderList(renderQueue);
        // add rotation component to the cube
        std::unique_ptr<ObjectRotationComponent> cubeRotationComponent = std::make_unique<ObjectRotationComponent>(4.0f);
        rayTraceObject8->addComponent(std::move(cubeRotationComponent));
        
        // the glass sphere (transparent object in rendering order)
        GSphere * sphereObject = new GSphere();
        sphereObject->setSkyboxTexture(skyboxTexture);
        RayTraceObject * rayTraceObject1 = new RayTraceObject(sphereObject,TRANSPARENT,renderContext);
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