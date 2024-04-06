#ifndef RAYTRACEOBJECT_H
#define RAYTRACEOBJECT_H

#include "GraphicObject.h"
#include "SceneObject.h"
#include <GPU_RAYTRACER/GPU_RAYTRACER.h>
#include <CPU_RAYTRACER/CPU_RAYTRACER.h>
#include <memory>

enum MaterialType {
    LAMBERTIAN = 0,
    METAL = 1,
    DIELECTRIC = 2,
    EMISSIVE = 3
};

class ObjectRenderComponent : public RenderComponent {
public:
    ObjectRenderComponent(GMVPObject * _obj, enum renderQueue _renderPriority = OPAQUE) {
        this->obj = _obj;
        renderPriority = _renderPriority;
    }
    void Render() override {
        obj->draw();
    }
private:
    GMVPObject * obj;
};





// RayTraceObject class, it is designed to connect the GObject class and the ray tracing pipeline in the GPU
// the main feature is to manage the data transfer between the CPU and the GPU: primitive, BVH, material, texture, model matrix, etc.
// it also in charge of detecting whether the scene has changed, so that GPU raytracer can update the scene data via updating TLAS
// instead of inheriting from GObject class, it contains an GObject pointer, and it can be constructed from an GObject pointer
class RayTraceObject : public RenderableSceneObject {
public:
    RayTraceObject(GMVPObject * _obj, enum renderQueue _renderPriority = OPAQUE) {
        this->obj = _obj;
        encodePrimitive();
        constructLocalBLAS();
        this->renderComponent = std::make_shared<ObjectRenderComponent>(obj,_renderPriority);
        
    }
    ~RayTraceObject() {
    }

    void setModelMatrix(glm::mat4 modelMatrix) {
        obj->setModel(modelMatrix);
        this->modelMatrix = modelMatrix;
        if (CPU_object_transform != nullptr) {
            CPU_object_transform->update_model_matrix(modelMatrix);
        }
    }

    glm::mat4 getModelMatrix() {
        return modelMatrix;
    }


    void setMaterial(int type, float fuzzOrIOR, glm::vec4 baseColor, Texture * _texture = nullptr) {
        material.type = type; // 0: lambertian, 1: metal, 2: dielectric
        // ensure if fuzziness, it is in the range [0, 1]
        if (type == 1) {
            material.fuzzOrIOR = glm::clamp(fuzzOrIOR, 0.0f, 1.0f);
        }
        else{
            material.fuzzOrIOR = fuzzOrIOR;
        }
        material.textureID = (texture == nullptr) ? -1 : 0; // -1 if no texture, 0 will later be changed to the texture ID in the texture array in raytrace_manager
        material.baseColor = glm::vec3(baseColor);
        obj->setColor(baseColor);
        // we specify the corresponding shader in default renderer to make it easy to manage the mapping between raytracing material type and rasterization shading shader 
        const char * vertexShaderPath = "shaders/texture_shader.vert";
        const char * fragmentShaderPath = "shaders/shader.frag";
        if (_texture != nullptr) {
            fragmentShaderPath = "shaders/texture_shader.frag";
            obj->setTexture(_texture);
            this->texture = _texture;
            // resize the texture to 1024x1024x3
            texture->resizeTexture(1024, 1024,3);
        }
        if (type == 0) {
            obj->setShader(new Shader(vertexShaderPath, fragmentShaderPath));
        }
        else if (type == 1) {
            obj->setShader(new Shader(vertexShaderPath, fragmentShaderPath));
        }
        else if (type == 2) {
            obj->setShader(new Shader(vertexShaderPath, fragmentShaderPath));
        }
        else if (type == 3) {
            obj->setShader(new Shader(vertexShaderPath, fragmentShaderPath));
        }
    
        // then inform the ray tracer to update the TLAS
        // ... not implemented yet
    }
   
    void update(){
        // GPU ray tracing components
        encodePrimitive();
        constructLocalBLAS();
        // CPU ray tracing components
        constructCPU_object();
    }
    
    // default rendering components
    GMVPObject * obj = nullptr; // the default object, it is used for forward rendering in the default renderer
    
    

    // though the model matrix is stored in the object, it is also stored here for easy access
    glm::mat4 modelMatrix = glm::mat4(1.0f); // the default model matrix is identity matrix
    // ------------------------------
    // GPU ray tracing components
    glm::vec3 AA, BB; // the AABB of the object
    std::vector<GPU_RAYTRACER::Primitive> localEncodedPrimitives;
    std::vector<GPU_RAYTRACER::BLASNode> localBLAS; // the root of the BLAS tree must be the first element of the vector
    GPU_RAYTRACER::Material material; // should later be changed to a pointer to a material object, so that the material can be shared among multiple objects 
    Texture * texture = nullptr;
    bool hasTexture() { return texture != nullptr; }
    Texture * getTexture() { return texture; }
    // ------------------------------
    // CPU ray tracing components
    shared_ptr<CPU_RAYTRACER::hittable> CPU_object = nullptr; // the hittable object for CPU ray tracing
    // if it has a transformation, the transformation node ref will be stored here
    // so that we can update the transformation matrix of the object by updating the transformation node
    shared_ptr<CPU_RAYTRACER::transform> CPU_object_transform = nullptr;
    // ------------------------------
    


    private:

    void constructCPU_object(){
        CPU_object = nullptr;
        shared_ptr<CPU_RAYTRACER::material> CPU_material = nullptr;
        // first check whether the material is a texture material
        shared_ptr<CPU_RAYTRACER::texture> CPU_texture = nullptr;
        if (this->texture != nullptr) {
            CPU_texture = make_shared<CPU_RAYTRACER::image_texture>(texture->getData(), texture->width, texture->height, texture->channels);
        }
        // then construct the material
        if (material.type == LAMBERTIAN) {
            CPU_material = make_shared<CPU_RAYTRACER::lambertian>(CPU_texture,material.baseColor);
        }
        else if (material.type == METAL) {
            CPU_material = make_shared<CPU_RAYTRACER::metal>(CPU_texture,material.baseColor, material.fuzzOrIOR);
        }
        else if (material.type == DIELECTRIC) {
            CPU_material = make_shared<CPU_RAYTRACER::dielectric>(material.fuzzOrIOR);
        }
        else if (material.type == EMISSIVE) {
            CPU_material = make_shared<CPU_RAYTRACER::diffuse_light>(CPU_texture,material.baseColor);
        }
        else{
            // unsupported material type, use lambertian with red color
            std::cout<<"unsupported material type"<<std::endl;
            CPU_material = make_shared<CPU_RAYTRACER::lambertian>(glm::vec3(1,0,0));
        }
        // then construct the CPU object
        if (dynamic_cast<GTriangle*>(obj)){
            GTriangle * triangle = dynamic_cast<GTriangle*>(obj);
            // construct the triangle object without transformation
            shared_ptr<CPU_RAYTRACER::hittable> primitiveObject = make_shared<CPU_RAYTRACER::triangle>(triangle->v0, triangle->v1, triangle->v2, CPU_material, triangle->t0, triangle->t1, triangle->t2, triangle->n0, triangle->n1, triangle->n2);\
            // then apply the transformation
            CPU_object = make_shared<CPU_RAYTRACER::transform>(primitiveObject, modelMatrix);
            // if the object has a transformation, store the transformation node ref
            // currently, only translation is supported
            CPU_object_transform = std::dynamic_pointer_cast<CPU_RAYTRACER::transform>(CPU_object);
        }
        else if (dynamic_cast<GSphere*>(obj)){
            GSphere * sphere = dynamic_cast<GSphere*>(obj);
            shared_ptr<CPU_RAYTRACER::hittable> primitiveObject = make_shared<CPU_RAYTRACER::sphere>(sphere->center, sphere->radius, CPU_material);
            CPU_object = make_shared<CPU_RAYTRACER::transform>(primitiveObject, modelMatrix);
            CPU_object_transform = std::dynamic_pointer_cast<CPU_RAYTRACER::transform>(CPU_object);

        }
        else if (dynamic_cast<GModel*>(obj)){
            GModel * model = dynamic_cast<GModel*>(obj);
            std::vector<shared_ptr<CPU_RAYTRACER::hittable>> hittable_triangles;
            for (GMesh* mesh : model->meshes){
                load_triangles(hittable_triangles, mesh->positions, mesh->uvs, mesh->normals, mesh->indices, CPU_material);
            }
            auto hittable_mesh = make_shared<CPU_RAYTRACER::mesh>(hittable_triangles);
            CPU_object = make_shared<CPU_RAYTRACER::transform>(hittable_mesh, modelMatrix);
            CPU_object_transform = std::dynamic_pointer_cast<CPU_RAYTRACER::transform>(CPU_object);
            
        }
        else{
            // unsupported object type
            std::cout<<"unsupported object type"<<std::endl;
        }

    }
    void updateCPU_object(){
        // not implemented yet, should be called when the object is updated (moved, rotated, scaled, deleted, etc.)
    }


    void encodePrimitive() {
        // construct local BLAS from the object
        // this function is called when the object is constructed, or when the object's primitives are changed
        // first check the type of the object via dynamic_cast: if it is a triangle, a sphere, or a model with multiple meshes
        localEncodedPrimitives.clear();
        localEncodedPrimitives.resize(0);
        if (dynamic_cast<GTriangle*>(obj)){
            GTriangle * triangle = dynamic_cast<GTriangle*>(obj);
            // encode the triangle to the primitive struct
            GPU_RAYTRACER::Primitive primitive;
            primitive.primitiveInfo = glm::vec3(0, 0, 0); // x: primitive type(0: triangle, 1: sphere), yz: reserved
            primitive.v0 = triangle->v0;
            primitive.v1 = triangle->v1;
            primitive.v2 = triangle->v2;
            primitive.n1 = triangle->n0;
            primitive.n2 = triangle->n1;
            primitive.n3 = triangle->n2;
            primitive.t1 = triangle->t0;
            primitive.t2 = triangle->t1;
            primitive.t3 = triangle->t2;
            localEncodedPrimitives.push_back(primitive);
            
        }
        else if (dynamic_cast<GSphere*>(obj)){
            GSphere * sphere = dynamic_cast<GSphere*>(obj);
            // encode the sphere to the primitive struct
            GPU_RAYTRACER::Primitive primitive;
            primitive.primitiveInfo = glm::vec3(1, 0, 0); // x: primitive type(0: triangle, 1: sphere), yz: reserved
            primitive.v0 = sphere->center; // center of the sphere
            primitive.v1 = glm::vec3(sphere->radius, 0, 0); // v1.x is the radius of the sphere
            primitive.v2 = glm::vec3(0,0,0);
            primitive.n1 = glm::vec3(0,0,0); // not used
            primitive.n2 = glm::vec3(0,0,0); // not used
            primitive.n3 = glm::vec3(0,0,0); // not used
            primitive.t1 = glm::vec3(0,1,0);    // not specified yet
            primitive.t2 = glm::vec3(1,1,0);
            primitive.t3 = glm::vec3(0.5,0.5,0);
            localEncodedPrimitives.push_back(primitive);
            
        }
        else if (dynamic_cast<GModel*>(obj)){
            GModel * model = dynamic_cast<GModel*>(obj);
            // encode the model to the primitive struct
            // loop through the triangles of the model on all the meshes
            for(GMesh* mesh : model->meshes){
                for (unsigned int i = 0; i < mesh->indices.size(); i+=3){
                    // encode the triangle to the primitive struct
                    GPU_RAYTRACER::Primitive primitive;
                    primitive.primitiveInfo = glm::vec3(0, 0, 0);
                    primitive.v0 = mesh->positions[mesh->indices[i]];
                    primitive.v1 = mesh->positions[mesh->indices[i+1]];
                    primitive.v2 = mesh->positions[mesh->indices[i+2]];
                    primitive.n1 = mesh->normals[mesh->indices[i]];
                    primitive.n2 = mesh->normals[mesh->indices[i+1]];
                    primitive.n3 = mesh->normals[mesh->indices[i+2]];
                    primitive.t1 = glm::vec3(mesh->uvs[mesh->indices[i]].x,mesh->uvs[mesh->indices[i]].y,0);
                    primitive.t2 = glm::vec3(mesh->uvs[mesh->indices[i+1]].x,mesh->uvs[mesh->indices[i+1]].y,0);
                    primitive.t3 = glm::vec3(mesh->uvs[mesh->indices[i+2]].x,mesh->uvs[mesh->indices[i+2]].y,0);
                    localEncodedPrimitives.push_back(primitive);
                }
            }
            
        }
        else{
            // unsupported object type
            std::cout<<"unsupported object type"<<std::endl;
        
        }
        
    }
    void constructLocalBLAS() {
        // construct local BLAS from the object
        // this function is called when the object is constructed, or when the object's primitives are changed
        // first check the type of the object via dynamic_cast: if it is a triangle, a sphere, or a model with multiple meshes
        localBLAS.clear();
        localBLAS.resize(0);
        if (dynamic_cast<GTriangle*>(obj)){
            GTriangle * triangle = dynamic_cast<GTriangle*>(obj);
            // calculate the AABB of the triangle
            AA = glm::min(triangle->v0, glm::min(triangle->v1, triangle->v2));
            BB = glm::max(triangle->v0, glm::max(triangle->v1, triangle->v2));
            // encode the triangle to the primitive struct
            GPU_RAYTRACER::BLASNode node;
            node.left = -1;
            node.right = -1;
            node.n = 1;
            node.index = 0; // the true index will be set when the TLAS is constructed in raytrace_manager
            node.AA = glm::vec4(AA, 0);
            node.BB = glm::vec4(BB, 0);
            localBLAS.push_back(node);// only one triangle, so only one BLAS node
        }
        else if (dynamic_cast<GSphere*>(obj)){
            GSphere * sphere = dynamic_cast<GSphere*>(obj);
            // calculate the AABB of the sphere
            glm::vec3 center = sphere->center;
            float radius = sphere->radius;
            AA = center - glm::vec3(radius, radius, radius);
            BB = center + glm::vec3(radius, radius, radius);
            // encode the sphere to the primitive struct
            GPU_RAYTRACER::BLASNode node;
            node.left = -1;
            node.right = -1;
            node.n = 1;
            node.index = 0; // the true index will be set when the TLAS is constructed in raytrace_manager
            node.AA = glm::vec4(AA, 0);
            node.BB = glm::vec4(BB, 0);
            localBLAS.push_back(node);// only one sphere, so only one BLAS node

        }
        else if (dynamic_cast<GModel*>(obj)){
            GModel * model = dynamic_cast<GModel*>(obj);
            // encode the model to the primitive struct
            // loop through the triangles of the model on all the meshes
            // similar to the TLAS construction, we need to construct the BLAS tree recursively in the index-array format
            buildBLASBVH(0, localEncodedPrimitives.size()-1);
            glm::vec3 rootAA = localBLAS[0].AA;
            glm::vec3 rootBB = localBLAS[0].BB;
            AA = rootAA;
            BB = rootBB;
            
        }
        else{
            // unsupported object type
            std::cout<<"unsupported object type"<<std::endl;
        }
    }
    
    // since the BLAS is relatively static in the scene, we can build it with a slower algorithm
    // but the resulting BVH should be more efficient in the ray tracing process
    // here we use SAH to build the BVH
    int buildBLASBVH(int left, int right, int maxTrianglesPerNode = 8) {
        using GPU_RAYTRACER::BLASNode;
        if (left > right) {
            std::cout<<"[buildBLASBVH]: error: left > right"<<std::endl;
            return -1;
        }
        BLASNode node;
        localBLAS.push_back(BLASNode());
        int idx = localBLAS.size()-1;
        if (right - left + 1 <= maxTrianglesPerNode) {
            // leaf node
            node.left = -1;
            node.right = -1;
            node.n = right - left + 1; // number of triangles in the node
            node.index = left; // point to the first triangle in the localEncodedPrimitives
            // calculate the AABB of the leaf node's trianglesq
            glm::vec3 AA = localEncodedPrimitives[left].v0;
            glm::vec3 BB = localEncodedPrimitives[left].v0;
            for (int i = left; i <= right; i++) {
                glm::vec3 v0 = localEncodedPrimitives[i].v0;
                glm::vec3 v1 = localEncodedPrimitives[i].v1;
                glm::vec3 v2 = localEncodedPrimitives[i].v2;
                AA = glm::min(AA, glm::min(v0, glm::min(v1, v2)));
                BB = glm::max(BB, glm::max(v0, glm::max(v1, v2)));
            }
            node.AA = glm::vec4(AA, 0);
            node.BB = glm::vec4(BB, 0);
            localBLAS[idx] = node;
            return idx;
        }
        // find the longest axis of the AABB
        else{
            // we now need to find the best split
            int bestAxis = 0;
            float bestCost = INFINITY;
            int bestSplit = (left+right)/2;
            for (int axis = 0; axis < 3; axis++) {
                // sort the triangles along the axis
                std::sort(localEncodedPrimitives.begin()+left, localEncodedPrimitives.begin()+right+1, [axis](const GPU_RAYTRACER::Primitive & a, const GPU_RAYTRACER::Primitive & b){
                    glm::vec3 centerA = 0.5f * (a.v0 + a.v1 + a.v2);
                    glm::vec3 centerB = 0.5f * (b.v0 + b.v1 + b.v2);
                    return centerA[axis] < centerB[axis];
                });

                // leftAAs[i] is the AA(min of xyz) of the triangles from left to i
                std::vector<glm::vec3> leftAAs(right-left+1, glm::vec3(INFINITY));
                std::vector<glm::vec3> leftBBs(right-left+1, glm::vec3(-INFINITY));
                
                for (int i = left; i <= right; i++) {
                    glm::vec3 v0 = localEncodedPrimitives[i].v0;
                    glm::vec3 v1 = localEncodedPrimitives[i].v1;
                    glm::vec3 v2 = localEncodedPrimitives[i].v2;

                    int bias = (i == left) ? 0 : 1;

                    leftAAs[i-left] = glm::min(leftAAs[i-left-bias], glm::min(v0, glm::min(v1, v2)));
                    leftBBs[i-left] = glm::max(leftBBs[i-left-bias], glm::max(v0, glm::max(v1, v2)));

                }

                // rightAAs[i] is the AA(min of xyz) of the triangles from i to right
                std::vector<glm::vec3> rightAAs(right-left+1, glm::vec3(INFINITY));
                std::vector<glm::vec3> rightBBs(right-left+1, glm::vec3(-INFINITY));

                for (int i = right; i >= left; i--) {
                    glm::vec3 v0 = localEncodedPrimitives[i].v0;
                    glm::vec3 v1 = localEncodedPrimitives[i].v1;
                    glm::vec3 v2 = localEncodedPrimitives[i].v2;

                    int bias = (i == right) ? 0 : 1;

                    rightAAs[i-left] = glm::min(rightAAs[i-left+bias], glm::min(v0, glm::min(v1, v2)));
                    rightBBs[i-left] = glm::max(rightBBs[i-left+bias], glm::max(v0, glm::max(v1, v2)));

                }

                // recursively calculate the cost of each split
                float cost = INFINITY;
                int split = left;
                for (int i = left; i < right; i++) {
                    glm::vec3 leftAA = leftAAs[i-left];
                    glm::vec3 leftBB = leftBBs[i-left];
                    glm::vec3 rightAA = rightAAs[i-left+1];
                    glm::vec3 rightBB = rightBBs[i-left+1];
                    glm::vec3 leftLength = leftBB - leftAA;
                    float leftArea = 2.0 * (leftLength.x * leftLength.y + leftLength.y * leftLength.z + leftLength.z * leftLength.x);
                    float leftCost = leftArea * (i - left + 1);
                    glm::vec3 rightLength = rightBB - rightAA;
                    float rightArea = 2.0 * (rightLength.x * rightLength.y + rightLength.y * rightLength.z + rightLength.z * rightLength.x);
                    float rightCost = rightArea * (right - i);
                    float splitCost = leftCost + rightCost;
                    if (splitCost < cost) {
                        cost = splitCost;
                        split = i;
                    }
                }
                if (cost < bestCost) {
                    bestCost = cost;
                    bestAxis = axis;
                    bestSplit = split;
                }
            }
            // sort the triangles along the best axis
            std::sort(localEncodedPrimitives.begin()+left, localEncodedPrimitives.begin()+right+1, [bestAxis](const GPU_RAYTRACER::Primitive & a, const GPU_RAYTRACER::Primitive & b){
                glm::vec3 centerA = 0.5f * (a.v0 + a.v1 + a.v2);
                glm::vec3 centerB = 0.5f * (b.v0 + b.v1 + b.v2);
                return centerA[bestAxis] < centerB[bestAxis];
            });
            // recursively build the left and right child
            int leftChild = buildBLASBVH(left, bestSplit, maxTrianglesPerNode);
            int rightChild = buildBLASBVH(bestSplit+1, right, maxTrianglesPerNode);
            node.left = leftChild;
            node.right = rightChild;
            node.n = 0; // not a leaf node
            node.index = -1; // not a leaf node
            // calculate the AABB of the node
            glm::vec3 AA = glm::vec3(FLT_MAX);
            glm::vec3 BB = glm::vec3(-FLT_MAX);
            if (leftChild != -1) {
                AA = glm::min(AA, glm::vec3(localBLAS[leftChild].AA));
                BB = glm::max(BB, glm::vec3(localBLAS[leftChild].BB));
            }
            if (rightChild != -1) {
                AA = glm::min(AA, glm::vec3(localBLAS[rightChild].AA));
                BB = glm::max(BB, glm::vec3(localBLAS[rightChild].BB));
            }
            if (leftChild == -1 && rightChild == -1) {
                std::cout<<"[buildBLASBVH]: error: both left and right child are -1"<<std::endl;
            }
            node.AA = glm::vec4(AA, 0);
            node.BB = glm::vec4(BB, 0);
            localBLAS[idx] = node;
            return idx;
        }
    }

    // just use random axis for this one
    int buildBLASBVHrandomAxis(int left, int right, int maxTrianglesPerNode = 8) {
        using GPU_RAYTRACER::BLASNode;
        if (left > right) {
            std::cout<<"[buildBLASBVH]: error: left > right"<<std::endl;
            return -1;
        }
        BLASNode node;
        localBLAS.push_back(BLASNode());
        int idx = localBLAS.size()-1;
        if (right - left + 1 <= maxTrianglesPerNode) {
            // leaf node
            node.left = -1;
            node.right = -1;
            node.n = right - left + 1; // number of triangles in the node
            node.index = left; // point to the first triangle in the localEncodedPrimitives
            // calculate the AABB of the leaf node's trianglesq
            glm::vec3 AA = localEncodedPrimitives[left].v0;
            glm::vec3 BB = localEncodedPrimitives[left].v0;
            for (int i = left; i <= right; i++) {
                glm::vec3 v0 = localEncodedPrimitives[i].v0;
                glm::vec3 v1 = localEncodedPrimitives[i].v1;
                glm::vec3 v2 = localEncodedPrimitives[i].v2;
                AA = glm::min(AA, glm::min(v0, glm::min(v1, v2)));
                BB = glm::max(BB, glm::max(v0, glm::max(v1, v2)));
            }
            node.AA = glm::vec4(AA, 0);
            node.BB = glm::vec4(BB, 0);
            localBLAS[idx] = node;
            return idx;
        }
        // sort the objects along a random axis
        // we can use the center of the AABB as the sorting key
        else{
            int axis = rand() % 3;
            std::sort(localEncodedPrimitives.begin() + left, localEncodedPrimitives.begin() + right + 1, [axis](const GPU_RAYTRACER::Primitive & a, const GPU_RAYTRACER::Primitive & b){
                glm::vec3 centerA = 0.5f * (a.v0 + a.v1 + a.v2);
                glm::vec3 centerB = 0.5f * (b.v0 + b.v1 + b.v2);
                return centerA[axis] < centerB[axis];
            });
            int mid = left + (right - left) / 2;
            int leftChild = buildBLASBVHrandomAxis(left, mid,maxTrianglesPerNode);
            int rightChild = buildBLASBVHrandomAxis(mid + 1, right,maxTrianglesPerNode);
            node.left = leftChild;
            node.right = rightChild;
            node.index = -1;
            node.n = 0;
            // calculate the AABB of the node
            glm::vec3 AA = glm::vec3(FLT_MAX);
            glm::vec3 BB = glm::vec3(-FLT_MAX);
            // instead of traversing every object between left and right, we can use the AABB of the left and right child nodes
            // to calculate the AABB of the current node
            if (leftChild != -1){
                AA = glm::min(AA, glm::vec3(localBLAS[leftChild].AA));
                BB = glm::max(BB, glm::vec3(localBLAS[leftChild].BB));
            }
            if (rightChild != -1){
                AA = glm::min(AA, glm::vec3(localBLAS[rightChild].AA));
                BB = glm::max(BB, glm::vec3(localBLAS[rightChild].BB));
            }
            node.AA = glm::vec4(AA, 0);
            node.BB = glm::vec4(BB, 0);
            localBLAS[idx] = node;
            return idx;
            
        }

    }


};

class RayTraceSkybox : public RenderableSceneObject {
public:
CPU_RAYTRACER::skybox * CPU_skybox;
GSkybox * skybox;

RayTraceSkybox(GSkybox * _skybox) {
    this->skybox = _skybox;
    this->renderComponent = std::make_shared<ObjectRenderComponent>(_skybox, SKYBOX);
    // copy the skybox data to CPU_skybox
    std::vector<void*> data_list; 
    std::vector<int> width_list; 
    std::vector<int> height_list; 
    std::vector<int> nrChannels_list;
    SkyboxTexture * _skyboxTexture = _skybox->skyboxTexture;
    for (int i = 0; i < 6; i++) {
        Texture * texture = &_skyboxTexture->faces[i];
        if (texture->dataFormat != GL_UNSIGNED_BYTE) {
            std::cout<<"skybox texture data format is not GL_UNSIGNED_BYTE"<<std::endl;
            return;
        }

        data_list.push_back(texture->getData());
        width_list.push_back(texture->width);
        height_list.push_back(texture->height);
        nrChannels_list.push_back(texture->channels);
    }
    CPU_skybox = new CPU_RAYTRACER::skybox(data_list, width_list, height_list, nrChannels_list);
}

    

    
};




#endif