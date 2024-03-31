#ifndef RAYTRACEOBJECT_H
#define RAYTRACEOBJECT_H

#include "Object.h"
#include "GPU_RAYTRACER/data_structures.h" // not a good practice, but for now, it is fine, it includes some data structures used in GPU raytracer


enum MaterialType {
    LAMBERTIAN = 0,
    METAL = 1,
    DIELECTRIC = 2
};




// RayTraceObject class, it is designed to connect the Object class and the ray tracing pipeline in the GPU
// the main feature is to manage the data transfer between the CPU and the GPU: primitive, BVH, material, texture, model matrix, etc.
// it also in charge of detecting whether the scene has changed, so that GPU raytracer can update the scene data via updating TLAS
// instead of inheriting from Object class, it contains an Object pointer, and it can be constructed from an Object pointer
class RayTraceObject{
public:
    RayTraceObject(MVPObject * obj) {
        this->obj = obj;
        encodePrimitive();
        constructLocalBLAS();
    }
    ~RayTraceObject() {
    }
    void setModelMatrix(glm::mat4 modelMatrix) {
        obj->setModel(modelMatrix);
        this->modelMatrix = modelMatrix;
        // then inform the ray tracer to update the TLAS
        // ... not implemented yet
    }
    void setMaterial(int type, float fuzzOrIOR, int textureID, glm::vec4 baseColor, const char * texturePath = "") {
        material.type = type; // 0: lambertian, 1: metal, 2: dielectric
        material.fuzzOrIOR = fuzzOrIOR;
        material.textureID = textureID;
        material.baseColor = glm::vec3(baseColor);
        obj->setColor(baseColor);
        // we specify the corresponding shader in default renderer to make it easy to manage the mapping between raytracing material type and rasterization shading shader 
        const char * vertexShaderPath = "../../shaders/texture_shader.vert";
        const char * fragmentShaderPath = "../../shaders/shader.frag";
        if (textureID != -1) {
            fragmentShaderPath = "../../shaders/texture_shader.frag";
            obj->setTexture(texturePath);
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
    
        // then inform the ray tracer to update the TLAS
        // ... not implemented yet
    }
    void setTexture() {
    }
    void updatePrimitive() {
    }
    void updateBLAS() {
    }
    void update(){
        encodePrimitive();
        constructLocalBLAS();
    }
    
    MVPObject * obj;
    glm::mat4 modelMatrix; // though the model matrix is stored in the object, it is also stored here for easy access
    glm::vec3 AA, BB; // the AABB of the object
    std::vector<GPU_RAYTRACER::Primitive> localEncodedPrimitives;
    std::vector<GPU_RAYTRACER::BLASNode> localBLAS; // the root of the BLAS tree must be the first element of the vector
    GPU_RAYTRACER::Material material; // should later be changed to a pointer to a material object, so that the material can be shared among multiple objects 
    // (material and texture management is not implemented yet)

    private:
    void encodePrimitive() {
        // construct local BLAS from the object
        // this function is called when the object is constructed, or when the object's primitives are changed
        // first check the type of the object via dynamic_cast: if it is a triangle, a sphere, or a model with multiple meshes
        localEncodedPrimitives.clear();
        localEncodedPrimitives.resize(0);
        if (dynamic_cast<Triangle*>(obj)){
            Triangle * triangle = dynamic_cast<Triangle*>(obj);
            // encode the triangle to the primitive struct
            GPU_RAYTRACER::Primitive primitive;
            primitive.primitiveInfo = glm::vec3(0, material.type, material.fuzzOrIOR); // x: primitive type(0: triangle, 1: sphere), y: material type(0: lambertian, 1: metal, 2: dielectric), z: fuzziness (if metal)
            primitive.baseColor = material.baseColor;
            primitive.v0 = triangle->v0;
            primitive.v1 = triangle->v1;
            primitive.v2 = triangle->v2;
            primitive.n1 = triangle->n1;
            primitive.n2 = triangle->n2;
            primitive.n3 = triangle->n3;
            primitive.t1 = glm::vec3(0,1,0);    // not specified yet
            primitive.t2 = glm::vec3(1,1,0);
            primitive.t3 = glm::vec3(0.5,0.5,0);
            localEncodedPrimitives.push_back(primitive);
            
        }
        else if (dynamic_cast<Sphere*>(obj)){
            Sphere * sphere = dynamic_cast<Sphere*>(obj);
            glm::quat rotation = glm::quat_cast(modelMatrix);
            // encode the sphere to the primitive struct
            GPU_RAYTRACER::Primitive primitive;
            primitive.primitiveInfo = glm::vec3(1, material.type, material.fuzzOrIOR); // x: primitive type(0: triangle, 1: sphere), y: material type(0: lambertian, 1: metal, 2: dielectric), z: fuzziness (if metal)
            primitive.baseColor = material.baseColor;
            primitive.v0 = glm::vec3(modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]);
            primitive.v1 = glm::vec3(1, 2, 3); // rotation quaternion's xyz
            primitive.v2 = glm::vec3(4, modelMatrix[0][0], 0); // rotation quaternion's w, and the radius (the sphere is scaled uniformly, so we can get the radius from any of the scale values)
            primitive.n1 = glm::vec3(0,0,0); // not used
            primitive.n2 = glm::vec3(0,0,0); // not used
            primitive.n3 = glm::vec3(0,0,0); // not used
            primitive.t1 = glm::vec3(0,1,0);    // not specified yet
            primitive.t2 = glm::vec3(1,1,0);
            primitive.t3 = glm::vec3(0.5,0.5,0);
            localEncodedPrimitives.push_back(primitive);
            
        }
        else if (dynamic_cast<Model*>(obj)){
            Model * model = dynamic_cast<Model*>(obj);
            // encode the model to the primitive struct
            // loop through the triangles of the model on all the meshes
            // not implemented yet
            
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
        if (dynamic_cast<Triangle*>(obj)){
            Triangle * triangle = dynamic_cast<Triangle*>(obj);
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
        else if (dynamic_cast<Sphere*>(obj)){
            Sphere * sphere = dynamic_cast<Sphere*>(obj);
            // calculate the AABB of the sphere
            glm::vec3 center = glm::vec3(sphere->model[3][0], sphere->model[3][1], sphere->model[3][2]);
            float radius = sphere->model[0][0]; // the sphere is scaled uniformly, so we can get the radius from any of the scale values
            //center = glm::vec3(0,0,0);
            //radius = 1;
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

            // this is a fucking stupid implementation only to enforce the sphere's local space equal to world space
            // if the sphere could be a sub element for other BLAS or skeletal mesh, this shit wont work anymore
            // I'll change this later...
            //modelMatrix = glm::mat4(1.0f);// the model matrix of the sphere is identity matrix, because the sphere is already in world space

        }
        else if (dynamic_cast<Model*>(obj)){
            Model * model = dynamic_cast<Model*>(obj);
            // encode the model to the primitive struct
            // loop through the triangles of the model on all the meshes
            // not implemented yet
            
        }
        else{
            // unsupported object type
            std::cout<<"unsupported object type"<<std::endl;
        
        }




    }

};

















#endif