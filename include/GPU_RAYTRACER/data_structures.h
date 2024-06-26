#ifndef GPU_DATA_STRUCTURES_H
#define GPU_DATA_STRUCTURES_H

#include <glm/glm.hpp>

namespace GPU_RAYTRACER{

    struct Material {
        int type; // 0: lambertian, 1: metal, 2: dielectric, 3: emissive
        float fuzzOrIOR; // fuzziness of the metal material or index of refraction of the dielectric material
        int textureID; // texture ID, -1 if no texture
        glm::vec3 baseColor;
    };
    
    // caution: we need to define the elements in glm::vec type (aka float) to be float
    // because we use float sampler (samplerBuffer) in shaders, which cannot interpret integer type (isamplerBuffer)
    struct BLASNode {
        // but actually these indices are integer, so we need to convert them back to integer in the shader
        float left, right;         // left and right child index, both -1 if it is a leaf node
        float n, index;            // number of triangles in the node, and the start index of the triangles in this node ( 0 and -1 if it is a leaf node)       
        glm::vec4 AA, BB;        // its AABB bounding box, vec4 for alignment
    };

    struct TLASNode {
        float left, right;         // left and right child index (if it is a leaf node, left and right are both -1)
        float BLASIndex;            // if it is a leaf node, it is the index of the BLAS node, otherwise it is -1
        float materialType;         // material type (0: lambertian, 1: metal, 2: dielectric)
        float textureID;            // texture ID, -1 if no texture
        glm::vec3 baseColor;     // base color of the material
        float fuzzOrIOR;         // fuzziness of the metal material or index of refraction of the dielectric material
        glm::vec3 AA, BB;        // its AABB bounding box
        float padding = -1;
        glm::mat4 modelMatrix;   // model matrix of the object
    };



    // encoded primitive data
    struct Primitive {
        glm::vec3 primitiveInfo; // x: primitive type(0: triangle, 1: sphere), y: material index (not implemented yet), z: reserved
        glm::vec3 v0, v1, v2;    // position (if sphere, v0: center, vec4(v1.xyz + v2.x): quaternion rotation, v2.y: radius)
        glm::vec3 n1, n2, n3;    // normal (if sphere, these are not used)
        glm::vec2 t1, t2, t3;    // t.xy: texture coordinate UV (if sphere, same, not used)
    };

    const GLuint PrimitiveSize = sizeof(Primitive); // should be 12 * 9 = 108 bytes
    const GLuint TLASNodeSize = sizeof(TLASNode); // should be 128
    const GLuint BLASNodeSize = sizeof(BLASNode); // should be 48

    const GLuint PrimitiveStride = PrimitiveSize / 12; // 12 is the size of a vec3 RGB32F, the stride of the primitive data in the buffer when using TBO to sample the data
    const GLuint TLASNodeStride = TLASNodeSize / 16; // TLAS TBO we use RGBA32F
    const GLuint BLASNodeStride = BLASNodeSize / 16; // BLAS TBO we use RGBA32F



    // some helper functions


    std::pair<glm::vec3,glm::vec3> transformAABB2WorldSpace(const glm::vec3& AA,const glm::vec3& BB, const glm::mat4& modelMatrix) {
        std::vector<glm::vec3> vertices = {
            glm::vec3(AA.x, AA.y, AA.z),
            glm::vec3(BB.x, AA.y, AA.z),
            glm::vec3(AA.x, BB.y, AA.z),
            glm::vec3(BB.x, BB.y, AA.z),
            glm::vec3(AA.x, AA.y, BB.z),
            glm::vec3(BB.x, AA.y, BB.z),
            glm::vec3(AA.x, BB.y, BB.z),
            glm::vec3(BB.x, BB.y, BB.z)
        };
        glm::vec3 new_AA = glm::vec3(modelMatrix * glm::vec4(vertices[0], 1.0f));
        glm::vec3 new_BB = new_AA;

        for (const auto& vertex : vertices) {
            glm::vec3 transformed_vertex = glm::vec3(modelMatrix * glm::vec4(vertex, 1.0f));
            new_AA = glm::min(new_AA, transformed_vertex);
            new_BB = glm::max(new_BB, transformed_vertex);
        }

        return std::make_pair(new_AA,new_BB);
        
    }



    // CUBE VERTICES（AABB FRAMEWIRE），scope [-1, 1]
    float cubeVertices[] = {
        // front
        -1.0f, -1.0f, -1.0f, 
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f, 
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, 
        -1.0f,  1.0f, -1.0f,  
        -1.0f, -1.0f, -1.0f, 
    
        // back
        -1.0f, -1.0f,  1.0f, 
         1.0f, -1.0f,  1.0f, 
         1.0f, -1.0f,  1.0f, 
         1.0f,  1.0f,  1.0f, 
         1.0f,  1.0f,  1.0f, 
        -1.0f,  1.0f,  1.0f,  
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f, 
    
        // 4 edge
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f, 
         1.0f, -1.0f,  1.0f, 
         1.0f,  1.0f, -1.0f, 
         1.0f,  1.0f,  1.0f,  
        -1.0f,  1.0f, -1.0f, 
        -1.0f,  1.0f,  1.0f  
    };

    


}

#endif
