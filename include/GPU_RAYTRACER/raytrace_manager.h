#ifndef GPU_RAYTRACER_RAYTRACE_MANAGER_H
#define GPU_RAYTRACER_RAYTRACE_MANAGER_H

#include "../Object.h"
#include "data_structures.h"
#include <vector>

namespace GPU_RAYTRACER{

    class RaytraceManager{
    public:
        RaytraceManager(int _width, int _height, const Camera * _camera, Rect * _screenCanvas) : camera(_camera), screenCanvas(_screenCanvas), width(_width), height(_height)
        {
            // generate the render texture
            glGenTextures(1, &renderTexture);
            glBindTexture(GL_TEXTURE_2D, renderTexture);
            // upload an green image for now, we will replace it with the result of the ray tracing
            std::vector<glm::vec4> greenImage(width * height, glm::vec4(0, 100, 0, 1));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, greenImage.data());

            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);


            // generate the texture buffer for the primitives
            glGenBuffers(1, &primitiveBuffer);
            glBindBuffer(GL_TEXTURE_BUFFER, primitiveBuffer);
            glGenTextures(1, &primitiveTexture);
            glBindTexture(GL_TEXTURE_BUFFER, primitiveTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, primitiveBuffer);
            glBindTexture(GL_TEXTURE_BUFFER, 0);


            // no bvh buffer for now


            // compile the compute shader
            setShaders();

        };
        void changeScreenSize(int _width, int _height){
            width = _width;
            height = _height;
            // resize the render texture
            glBindTexture(GL_TEXTURE_2D, renderTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
        };

        ~RaytraceManager(){

        };
        // include skybox, objects, dynamic_cast to check the type of the object
        // and store the object in the corresponding list (triangles, spheres, skybox)
        void loadScene(std::vector<Object*> objects){
            // clear the previous data
            encodedPrimitives.clear();
            bvhNodes.clear();
            // loop through the objects
            for (int i = 0; i < objects.size(); i++){
                if (dynamic_cast<Triangle*>(objects[i])){
                    Triangle * triangle = dynamic_cast<Triangle*>(objects[i]);
                    // encode the triangle to the primitive struct
                    Primitive primitive;
                    primitive.primitiveInfo = glm::vec3(0, 0, 0); // x: primitive type(0: triangle, 1: sphere), y: material type(0: lambertian, 1: metal, 2: dielectric), z: fuzziness (if metal)
                    primitive.baseColor = triangle->color;
                    primitive.v0 = triangle->v0;
                    primitive.v1 = triangle->v1;
                    primitive.v2 = triangle->v2;
                    primitive.n1 = triangle->n1;
                    primitive.n2 = triangle->n2;
                    primitive.n3 = triangle->n3;
                    primitive.t1 = glm::vec3(0,1,0);    // not specified yet
                    primitive.t2 = glm::vec3(1,1,0);
                    primitive.t3 = glm::vec3(0.5,0.5,0);
                    encodedPrimitives.push_back(primitive);
                    
                }
                else if (dynamic_cast<Sphere*>(objects[i])){
                    Sphere * sphere = dynamic_cast<Sphere*>(objects[i]);
                    glm::mat4 model = sphere->model;
                    glm::quat rotation = glm::quat_cast(model);
                    // encode the sphere to the primitive struct
                    Primitive primitive;
                    primitive.primitiveInfo = glm::vec3(1, 0, 0); // 1: sphere, 0: lambertian
                    primitive.baseColor = sphere->color;
                    primitive.v0 = glm::vec3(model[3][0], model[3][1], model[3][2]);
                    primitive.v1 = glm::vec3(1, 2, 3); // rotation quaternion's xyz
                    primitive.v2 = glm::vec3(4, model[0][0], 0); // rotation quaternion's w, and the radius (the sphere is scaled uniformly, so we can get the radius from any of the scale values)
                    primitive.n1 = glm::vec3(0,0,0); // not used
                    primitive.n2 = glm::vec3(0,0,0); // not used
                    primitive.n3 = glm::vec3(0,0,0); // not used
                    primitive.t1 = glm::vec3(0,1,0);    // not specified yet
                    primitive.t2 = glm::vec3(1,1,0);
                    primitive.t3 = glm::vec3(0.5,0.5,0);
                    encodedPrimitives.push_back(primitive);
                    
                }
                else if (dynamic_cast<Skybox*>(objects[i])){
                    // we just need its texture
                    Skybox * skybox = dynamic_cast<Skybox*>(objects[i]);
                    hasSkybox = true;
                    skyboxTexture = skybox->texture;
                }
                else if (dynamic_cast<Model*>(objects[i])){
                    Model * model = dynamic_cast<Model*>(objects[i]);
                    // encode the model to the primitive struct
                    // loop through the triangles of the model on all the meshes
                    // not implemented yet
                    
                }
                else{
                    // unsupported object type
                    std::cout<<"unsupported object type"<<std::endl;
                
                }
            }
            // construct BVH tree
            //constructBVH();
            // upload the scene data to the GPU
            uploadSceneData();

        };
        // with its gpu ray tracing shader, render the scene (two passes: first pass for ray tracing, second pass for displaying the result)
        // the first pass: it will write the result to the 'renderTexture'
        void compute(){
            // increment the frame counter
            frameCounter++;
            // set the uniforms for the compute shader (update the camera, frame count, screen size, etc.)
            updateUniforms();
            // bind the compute shader
            raytraceComputeShader->use();
            // bind the render texture
            glBindImageTexture(2, renderTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            // bind the skybox texture to texture unit 1
            if (hasSkybox){
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            }
            // bind the primitive texture to texture unit 0
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_BUFFER, primitiveTexture);

            // dispatch the compute shader, local size is 16x16x1
            glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            // unbind the render texture
            glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


        };
        // construct BVH tree for the scene (in CPU, then upload to GPU)
        void constructBVH();
        // upload scene data to gpu: bvh nodes, objects(geometries(triangles/spheres), materials)
        void uploadSceneData(){
            // upload the primitives
            glBindBuffer(GL_TEXTURE_BUFFER, primitiveBuffer);
            glBufferData(GL_TEXTURE_BUFFER, encodedPrimitives.size() * PrimitiveSize, encodedPrimitives.data(), GL_STATIC_DRAW);
            glActiveTexture(GL_TEXTURE0); // primitive texture will be bound to texture unit 0, we specify this in the shader 
            glBindTexture(GL_TEXTURE_BUFFER, primitiveTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, primitiveBuffer);
            raytraceComputeShader->use();
            raytraceComputeShader->setInt("primitiveBuffer", 0);
            glBindTexture(GL_TEXTURE_BUFFER, 0);
            // upload the bvh nodes
            // TLAS: top level acceleration structure, can be updated frequently as scene changes or meshes move



            // BLAS: bottom level acceleration structure, should be static, can be updated rarely, e.g. when a mesh is added or removed


            

            // upload the skybox texture
            if (hasSkybox){
                glActiveTexture(GL_TEXTURE1); // skybox texture will be bound to texture unit 1, we specify this in the shader
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
                // set the sampler for the skybox texture
                raytraceComputeShader->setInt("skyboxTexture", 1);
            }
            // set the uniforms for the compute shader
            updateUniforms();

            
        };


        void updateUniforms(){
            // set the uniforms for the compute shader
            raytraceComputeShader->use();
            raytraceComputeShader->setInt("primitiveTexture", 0); // primitive texture is bound to texture unit 0
            raytraceComputeShader->setInt("skyboxTexture", 1); // skybox texture is bound to texture unit 1
            raytraceComputeShader->setVec3("cameraPos", camera->Position);
            raytraceComputeShader->setVec3("cameraFront", camera->Front);
            raytraceComputeShader->setVec3("cameraUp", camera->Up);
            raytraceComputeShader->setFloat("fov", camera->Zoom);
            raytraceComputeShader->setFloat("aspectRatio", (float)width / (float)height);
            raytraceComputeShader->setBool("hasSkybox", hasSkybox);
            raytraceComputeShader->setInt("imageWidth", width);
            raytraceComputeShader->setInt("imageHeight", height);
            raytraceComputeShader->setInt("primitiveCount", encodedPrimitives.size());
            raytraceComputeShader->setInt("bvhNodeCount", bvhNodes.size());
            raytraceComputeShader->setInt("maxDepth", 5);
            raytraceComputeShader->setInt("frameCounter", frameCounter);
            // upload a time
            float time = glfwGetTime();
            raytraceComputeShader->setFloat("time", time);

            //std::cout<<"primitive count: "<<encodedPrimitives.size()<<std::endl;
        };
        


        // set shaders for the ray tracing pipeline: compute shader for ray tracing, vertex/fragment shaders for displaying the result
        // hard coded shaders for now, since we are not going to change them
        void setShaders(){
            // compute shader
            raytraceComputeShader = new ComputeShader("../../shaders/raytrace_compute_shader.comp");
            

        };
        void setScreenCanvas(Rect * _screenCanvas){
            screenCanvas = _screenCanvas;
        };

        // activate the GPU ray tracing pipeline, it will bind the shader for screenCanvas, so that
        // visualizing the result of the ray tracing will be possible from outside by letting the screenCanvas to render
        void activate(){
            screenCanvas->setTexture(renderTexture);
        };
        // deactivate the GPU ray tracing pipeline, it will unbind the shader for screenCanvas
        void deactivate(){
            screenCanvas->texture = 0;
            screenCanvas->hasTexture = false;
        }
        void resetFrameCounter(){
            frameCounter = 0;
        };
    private:
        GLuint renderTexture; // texture to render the result of the ray tracing
        GLuint bvhBuffer; // ssbo for bvh nodes
        GLuint primitiveBuffer; // tbo for primitives (triangles/spheres)
        GLuint primitiveTexture; // texture buffer for the primitives
        bool hasSkybox = false;
        GLuint skyboxTexture; // skybox texture
        
        std::vector<Primitive> encodedPrimitives; // triangles and spheres
        std::vector<BVHNode> bvhNodes;
        // shaders
        Shader * raytraceComputeShader;
        Shader * displayVertexShader;// not used for now, we render from outside using the screenCanvas
        Shader * displayFragmentShader;// not used for now
        // ref to screen quad object 
        Rect * screenCanvas;
        // ref to camera object, to get the view matrix and the projection matrix
        // to figure the ray directions in the compute shader
        const Camera * camera;
        // width and height of the render texture
        int width, height;
        int frameCounter = 0;
        



    };















}


#endif 