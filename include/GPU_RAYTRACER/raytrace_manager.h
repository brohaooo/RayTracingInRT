#ifndef GPU_RAYTRACER_RAYTRACE_MANAGER_H
#define GPU_RAYTRACER_RAYTRACE_MANAGER_H

#include "../RayTraceObject.h"
#include "data_structures.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>


namespace GPU_RAYTRACER{

    class RaytraceManager{
    public:
        RaytraceManager(int _width, int _height, const Camera * _camera, Rect * _screenCanvas) : camera(_camera), screenCanvas(_screenCanvas), width(_width), height(_height)
        {
            // generate the render texture
            renderTexture = new TextureRenderTarget(GL_FLOAT, GL_RGBA32F);            


            // upload an green image for now, we will replace it with the result of the ray tracing
            // std::vector<glm::vec4> * greenImage = new std::vector<glm::vec4>(width * height, glm::vec4(0, 100, 0, 1));
            // we use void * greenImage from heap to manually manage the memory:
            void * greenImage = new glm::vec4[width * height];
            for (int i = 0; i < width * height; i++){
                ((glm::vec4*)greenImage)[i] = glm::vec4(0, 100, 0, 1);
            }
            //renderTexture->loadFromData(width, height, 4,(greenImage));
            renderTexture->setSize(width, height, 4);
            renderTexture->createGPUTexture();
            renderTexture->updateGPUTexture(greenImage);

            // generate the texture buffer for the primitives
            glGenBuffers(1, &primitiveBuffer);
            glBindBuffer(GL_TEXTURE_BUFFER, primitiveBuffer);
            glGenTextures(1, &primitiveTexture);
            glBindTexture(GL_TEXTURE_BUFFER, primitiveTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, primitiveBuffer);
            glBindTexture(GL_TEXTURE_BUFFER, 0);


            // generate the TLAS buffer and BLAS buffer, still use the texture buffer for the TLAS and BLAS
            glGenBuffers(1, &TLASBuffer);
            glBindBuffer(GL_TEXTURE_BUFFER, TLASBuffer);
            glGenTextures(1, &TLASTexture);
            glBindTexture(GL_TEXTURE_BUFFER, TLASTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, TLASBuffer);// vec4 would be more efficient
            glBindTexture(GL_TEXTURE_BUFFER, 0);

            glGenBuffers(1, &BLASBuffer);
            glBindBuffer(GL_TEXTURE_BUFFER, BLASBuffer);
            glGenTextures(1, &BLASTexture);
            glBindTexture(GL_TEXTURE_BUFFER, BLASTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, BLASBuffer);// vec4 would be more efficient
            glBindTexture(GL_TEXTURE_BUFFER, 0);

            // generate the texture array for the scene textures
            glGenTextures(1, &sceneTextureArray);
            glBindTexture(GL_TEXTURE_2D_ARRAY, sceneTextureArray);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);


            // compile the compute shader
            setShaders();

        };
        void changeScreenSize(int _width, int _height){
            width = _width;
            height = _height;
            // resize the render texture
            //std::cout<<"resize render texture"<<std::endl;
            renderTexture->resizeTexture(width, height, 4);
            //glBindTexture(GL_TEXTURE_2D, renderTexture->getTextureRef());
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            //glBindTexture(GL_TEXTURE_2D, 0);
        };

        ~RaytraceManager(){

        };
        // include skybox, objects, dynamic_cast to check the type of the object
        // and store the object in the corresponding list (triangles, spheres, skybox)
        void loadScene(std::vector<RayTraceObject*> _rayTraceObjects, SkyboxTexture* _skyboxTexture){
            // clear the previous data
            encodedPrimitives.clear();
            this->rayTraceObjects = _rayTraceObjects;
            // put all the primitives in the scene to the encodedPrimitives list
            for (int i = 0; i < rayTraceObjects.size(); i++){
                RayTraceObject * obj = rayTraceObjects[i];
                int currentPrimitiveIndex = encodedPrimitives.size();
                objectPrimitiveIndex[obj] = currentPrimitiveIndex;
                encodedPrimitives.insert(encodedPrimitives.end(), obj->localEncodedPrimitives.begin(), obj->localEncodedPrimitives.end());
                // if has texture, add it to the textureIDMap, and set the textureID of the material
                if (obj->hasTexture()){
                    // add the texture to the textureIDMap
                    addTexture(obj->getTexture());
                    // set the textureID of the material
                    obj->material.textureID = textureIDMap[obj->getTexture()];
                }
            }


            
            if (_skyboxTexture != nullptr){
                hasSkybox = true;
                this->skyboxTexture = _skyboxTexture;
            }
            // construct BVH tree
            constructBVH();
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
            glBindImageTexture(2, renderTexture->getTextureRef(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            // bind the skybox texture to texture unit 1
            if (hasSkybox){
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture->getTextureRef());
            }
            // bind the primitive texture to texture unit 0
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_BUFFER, primitiveTexture);
            // bind the TLAS texture to texture unit 2
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_BUFFER, TLASTexture);
            // bind the BLAS texture to texture unit 3
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_BUFFER, BLASTexture);

            // bind the scene texture array to texture unit 4
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D_ARRAY, sceneTextureArray);
            

            // dispatch the compute shader, local size is 16x16x1
            glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            // unbind the render texture
            glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


        };
        // construct BVH tree for the scene (in CPU, then upload to GPU)
        void constructBVH(){
            TLASNodes.clear();
            BLASNodes.clear();
            // construct the bottom level acceleration structure
            constructBLAS();
            // construct the top level acceleration structure
            constructTLAS();
        }

        // recursive function to build the BVH tree, stored as array of nodes
        // return the index of the root node of the BVH tree
        // during the construction, we will sort the rayTraceObjects recursively
        int buildTLASBVH(int left, int right){
            if (left > right){
                std::cout<< "[buildTLASBVH]: error: left > right"<<std::endl;// this should not happen
                return -1;
            }
            // in each recursive call, we insert a node to the TLASNodes list
            TLASNode node;
            TLASNodes.push_back(TLASNode());
            int idx = TLASNodes.size() - 1;
            if (left == right){// it should be a leaf node, containing Model matrix and world space AABB, pointing to the BLAS node, which is the root of the local BLAS tree
                node.left = -1;
                node.right = -1;
                RayTraceObject * obj = rayTraceObjects[left];
                node.BLASIndex = objectBLASIndex[obj];
                // this node's AABB is the world space AABB of the object (multiplied by the model matrix)
                auto result = transformAABB2WorldSpace(obj->AA, obj->BB, obj->modelMatrix);
                node.AA = result.first;
                node.BB = result.second;
                node.materialType = obj->material.type;
                node.textureID = obj->material.textureID;
                node.baseColor = obj->material.baseColor;
                node.fuzzOrIOR = obj->material.fuzzOrIOR;
                node.modelMatrix = obj->modelMatrix;
                TLASNodes[idx] = node;
                return idx;
            }
            // sort the objects along a random axis
            // we can use the center of the AABB as the sorting key
            else{
                int axis = rand() % 3;
                std::sort(rayTraceObjects.begin() + left, rayTraceObjects.begin() + right + 1, [axis](RayTraceObject * a, RayTraceObject * b){
                    glm::vec3 centerA = 0.5f * (a->AA + a->BB);// same as (a->localBLAS[0].AA + a->localBLAS[0].BB)
                    glm::vec3 centerB = 0.5f * (b->AA + b->BB);// same as (b->localBLAS[0].AA + b->localBLAS[0].BB)
                    return centerA[axis] < centerB[axis];
                });
                int mid = left + (right - left) / 2;
                int leftChild = buildTLASBVH(left, mid);
                int rightChild = buildTLASBVH(mid + 1, right);
                node.left = leftChild;
                node.right = rightChild;
                node.BLASIndex = -1;
                // calculate the AABB of the node
                glm::vec3 AA = glm::vec3(FLT_MAX);
                glm::vec3 BB = glm::vec3(-FLT_MAX);
                // instead of traversing every object between left and right, we can use the AABB of the left and right child nodes
                // to calculate the AABB of the current node
                if (leftChild != -1){
                    AA = glm::min(AA, TLASNodes[leftChild].AA);
                    BB = glm::max(BB, TLASNodes[leftChild].BB);
                }
                if (rightChild != -1){
                    AA = glm::min(AA, TLASNodes[rightChild].AA);
                    BB = glm::max(BB, TLASNodes[rightChild].BB);
                }
                node.AA = AA;
                node.BB = BB;
                TLASNodes[idx] = node;
                return idx;

            }
            
        }


        void constructTLAS(){
            buildTLASBVH(0, rayTraceObjects.size() - 1);
        };

        void updateTLAS(){
            int oldTLASNodeCount = TLASNodes.size();
            TLASNodes.clear();
            constructTLAS();
            if (oldTLASNodeCount == TLASNodes.size()){
                updateTLASData();
            }
            else{
                uploadTLASData();
            }
        }


        void constructBLAS(){
            // just use the local BLAS nodes of each object to the BLASNodes list
            // but we need to calculate the shifted index for each BLAS nodes
            for (int i = 0; i < rayTraceObjects.size(); i++){
                RayTraceObject * obj = rayTraceObjects[i];
                int currentPrimitiveIndex = objectPrimitiveIndex[obj];
                int currentBLASIndex = BLASNodes.size();
                std::vector<BLASNode> & localBLASNodes = obj->localBLAS;
                for (int j = 0; j < localBLASNodes.size(); j++){
                    BLASNode node = localBLASNodes[j];
                    node.index += currentPrimitiveIndex;
                    if (node.left != -1){
                        node.left += currentBLASIndex;
                    }
                    if (node.right != -1){
                        node.right += currentBLASIndex;
                    }
                    BLASNodes.push_back(node);
                }
                objectBLASIndex[obj] = currentBLASIndex;
                
            }

        };


        void draw_TLAS_AABB(){
            // draw the AABB of the TLAS nodes
            for (int i = 0; i < TLASNodes.size(); i++){
                //std::cout<<"TLAS node "<<i<<std::endl;
                TLASNode node = TLASNodes[i];
                //if (node.BLASIndex == -1){
                //    continue;
                //}
                // these AABBs are in world space
                glm::vec3 AA = node.AA;
                glm::vec3 BB = node.BB;
                debugAABBShader->use();
                debugAABBShader->setVec3("AABB_min", AA);
                debugAABBShader->setVec3("AABB_max", BB);
                // set the model matrix to identity because the AABB of the TLAS nodes are already in world space
                glm::mat4 model = glm::mat4(1.0f);
                debugAABBShader->setMat4("model", model);
                debugAABBShader->setVec4("lineColor", glm::vec4(1, 1, 1, 1));

                glBindVertexArray(AABB_VAO);
                glDrawArrays(GL_LINES, 0, 24);
            }
        };

        void draw_BLAS_AABB(){
            // draw the AABB of the BLAS nodes
            for(RayTraceObject * obj : rayTraceObjects){
                int BLASIndex = objectBLASIndex[obj];
                glm::mat4 model = obj->modelMatrix;
                for (int i = 0; i < obj->localBLAS.size(); i++){
                    BLASNode node = obj->localBLAS[i+BLASIndex];
                    //if (node.n == 0){
                    //    continue;
                    //}
                    // these AABBs are in world space
                    glm::vec3 AA = node.AA;
                    glm::vec3 BB = node.BB;
                    debugAABBShader->use();
                    debugAABBShader->setVec3("AABB_min", AA);
                    debugAABBShader->setVec3("AABB_max", BB);
                    // set the model matrix
                    debugAABBShader->setMat4("model", model);
                    debugAABBShader->setVec4("lineColor", glm::vec4(0, 1, 0, 1));

                    glBindVertexArray(AABB_VAO);
                    glDrawArrays(GL_LINES, 0, 24);
                }
            }

        };
        


        // upload scene data to gpu: bvh nodes, objects(geometries(triangles/spheres), materials)
        void uploadSceneData(){
            // upload the primitives
            uploadPrimitiveData();
            // upload the bvh nodes
            // TLAS: top level acceleration structure, can be updated frequently as scene changes or meshes move
            uploadTLASData();
            // BLAS: bottom level acceleration structure, should be static, can be updated rarely, e.g. when a mesh is added or removed
            uploadBLASData();
            // upload the scene textures
            uploadSceneTextureArray();

            // upload the skybox texture
            if (hasSkybox){
                glActiveTexture(GL_TEXTURE1); // skybox texture will be bound to texture unit 1, we specify this in the shader
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture->getTextureRef());
                // set the sampler for the skybox texture
                raytraceComputeShader->setInt("skyboxTexture", 1);
            }
            // set the uniforms for the compute shader
            updateUniforms();
        };
        
        void uploadPrimitiveData(){
            // upload the primitives
            glBindBuffer(GL_TEXTURE_BUFFER, primitiveBuffer);
            glBufferData(GL_TEXTURE_BUFFER, encodedPrimitives.size() * PrimitiveSize, encodedPrimitives.data(), GL_STATIC_DRAW);
        };
        // TLAS: top level acceleration structure, can be updated frequently as scene changes or meshes move
        void uploadTLASData(){
            // upload the TLAS nodes
            glBindBuffer(GL_TEXTURE_BUFFER, TLASBuffer);
            glBufferData(GL_TEXTURE_BUFFER, TLASNodes.size() * TLASNodeSize, TLASNodes.data(), GL_DYNAMIC_DRAW);
        };

        // BLAS: bottom level acceleration structure, should be static, can be updated rarely, e.g. when a mesh is added or removed
        void uploadBLASData(){
            // upload the BLAS nodes
            glBindBuffer(GL_TEXTURE_BUFFER, BLASBuffer);
            glBufferData(GL_TEXTURE_BUFFER, BLASNodes.size() * BLASNodeSize, BLASNodes.data(), GL_STATIC_DRAW);
        };
        // if the TLAS remains the same size, we can use glBufferSubData to update the TLAS data
        // otherwise, we need to call uploadTLASData() to re-allcate the buffer and re-upload the data
        void updateTLASData(){
            glBindBuffer(GL_TEXTURE_BUFFER, TLASBuffer);
            glBufferSubData(GL_TEXTURE_BUFFER, 0, TLASNodes.size() * TLASNodeSize, TLASNodes.data());
        };


        void updateUniforms(){
            // set the uniforms for the compute shader
            raytraceComputeShader->use();
            raytraceComputeShader->setInt("primitives", 0); // primitive texture is bound to texture unit 0
            raytraceComputeShader->setInt("skyboxTexture", 1); // skybox texture is bound to texture unit 1
            raytraceComputeShader->setInt("TLAS", 2); // TLAS texture is bound to texture unit 2
            raytraceComputeShader->setInt("BLAS", 3); // BLAS texture is bound to texture unit 3
            raytraceComputeShader->setInt("sceneTextures", 4); // scene texture array is bound to texture unit 4
            raytraceComputeShader->setVec3("cameraPos", camera->Position);
            raytraceComputeShader->setVec3("cameraFront", camera->Front);
            raytraceComputeShader->setVec3("cameraUp", camera->Up);
            raytraceComputeShader->setFloat("fov", camera->Zoom);
            raytraceComputeShader->setFloat("aspectRatio", (float)width / (float)height);
            raytraceComputeShader->setBool("hasSkybox", hasSkybox);
            raytraceComputeShader->setInt("imageWidth", width);
            raytraceComputeShader->setInt("imageHeight", height);
            raytraceComputeShader->setInt("primitiveCount", encodedPrimitives.size());
            raytraceComputeShader->setInt("maxDepth", 4);
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
            raytraceComputeShader = new ComputeShader("shaders/raytrace_compute_shader.comp");
            // debug AABB shader
            debugAABBShader = new Shader("shaders/debug_AABB.vert", "shaders/debug_AABB.frag");
            // set up the VAO and VBO for drawing AABBs
            glGenVertexArrays(1, &AABB_VAO);
            glGenBuffers(1, &AABB_VBO);
            glBindVertexArray(AABB_VAO);
            glBindBuffer(GL_ARRAY_BUFFER, AABB_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);



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
        // void deactivate(){
        //     screenCanvas->texture = 0;
        //     screenCanvas->hasTexture = false;
        // }
        void resetFrameCounter(){
            frameCounter = 0;
        };
    private:
        TextureRenderTarget * renderTexture; // render texture object
        GLuint TLASBuffer; // top level acceleration structure buffer
        GLuint TLASTexture; // texture buffer for the TLAS
        GLuint BLASBuffer; // bottom level acceleration structure buffer
        GLuint BLASTexture; // texture buffer for the BLAS
        GLuint primitiveBuffer; // tbo for primitives (triangles/spheres)
        GLuint primitiveTexture; // texture buffer for the primitives
        bool hasSkybox = false;
        SkyboxTexture * skyboxTexture; // skybox texture

        std::vector<RayTraceObject*> rayTraceObjects; // objects in the scene
        std::unordered_map<RayTraceObject*, int> objectPrimitiveIndex; // map from object ptr to its Primitive starting index(shift amount in the primitive buffer)
        std::unordered_map<RayTraceObject*, int> objectBLASIndex; // map from object ptr to its BLAS starting index(shift amount in the BLAS buffer)
        std::vector<Primitive> encodedPrimitives; // triangles and spheres
        std::vector<TLASNode> TLASNodes;
        std::vector<BLASNode> BLASNodes;
        // shaders
        Shader * raytraceComputeShader;
        Shader * debugAABBShader; // shader for drawing AABBs
        GLuint AABB_VAO, AABB_VBO; // VAO and VBO for drawing AABBs

        // ref to screen quad object 
        Rect * screenCanvas;
        // ref to camera object, to get the view matrix and the projection matrix
        // to figure the ray directions in the compute shader
        const Camera * camera;
        // width and height of the render texture
        int width, height;
        int frameCounter = 0;
        
        // a mapping between the Texture object and the texture ID in the raytrace_manager's texture array from 0 to N
        std::unordered_map<Texture*, int> textureIDMap;
        std::unordered_set<int> textureIDSet;
        const int maxTextureCount = 16;

        void addTexture(Texture * texture){
            if (textureIDMap.find(texture) == textureIDMap.end()){
                bool inserted = false;
                for (int i = 0; i < maxTextureCount; i++){
                    if (textureIDSet.find(i) == textureIDSet.end()){
                        textureIDSet.insert(i);
                        textureIDMap[texture] = i;
                        inserted = true;
                        break;
                    }
                }
                if (!inserted){
                    std::cout<<"[addTexture]: error: texture array is full"<<std::endl;
                }
            }
        };

        void removeTexture(Texture * texture){
            if (textureIDMap.find(texture) != textureIDMap.end()){
                int textureID = textureIDMap[texture];
                textureIDSet.erase(textureID);
                textureIDMap.erase(texture);
            }
        };

        GLuint sceneTextureArray; // texture array for the scene textures

        void uploadSceneTextureArray(){
            // use the textureIDMap to upload the texture array to the compute shader
            int textureCount = textureIDMap.size();
            glBindTexture(GL_TEXTURE_2D_ARRAY, sceneTextureArray);
            // we enforce the texture size to be 1024x1024 and the format to be GL_RGB in the raytracing texture array
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, 1024, 1024, textureCount, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            // upload the textures to the texture array
            for (auto it = textureIDMap.begin(); it != textureIDMap.end(); it++){
                Texture * texture = it->first;
                int textureID = it->second;
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, textureID, 1024, 1024, 1, GL_RGB, GL_UNSIGNED_BYTE, texture->getData());
            }


        }


    };

    


}


#endif 