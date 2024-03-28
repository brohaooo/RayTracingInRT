#ifndef GPU_DATA_STRUCTURES_H
#define GPU_DATA_STRUCTURES_H

#include <glm/glm.hpp>

namespace GPU_RAYTRACER{

    //struct Material {
    //    int type; // 0: lambertian, 1: metal, 2: dielectric
    //    float fuzz; // fuzziness of the metal material
    //    glm::vec3 baseColor;
    //};
    //// triangle mesh
    //struct Triangle {
    //    glm::vec3 v0, v1, v2;    // position
    //    glm::vec3 n1, n2, n3;    // normal
    //    glm::vec2 t1, t2, t3;    // texture coordinate UV
    //    Material material;       // material (per triangle)
    //};
    //// sphere (individual, not in a mesh) // currently not used
    //struct Sphere {
    //    glm::vec3 center;        // center position
    //    float radius;            // radius
    //    Material material;       // material (per sphere)
    //};

    // BVH tree node
    struct BVHNode {
        int left, right;         // left and right child index
        int n, index;            // number of triangles in the node, and the start index of the triangles in this node               
        glm::vec3 AA, BB;        // its AABB bounding box
    };
    

    // encoded primitive data
    struct Primitive {
        glm::vec3 primitiveInfo; // x: primitive type(0: triangle, 1: sphere), y: material type(0: lambertian, 1: metal, 2: dielectric), z: fuzziness (if metal)
        glm::vec3 baseColor;     // base color of the material
        glm::vec3 v0, v1, v2;    // position (if sphere, v0: center, vec4(v1.xyz + v2.x): quaternion rotation, v2.y: radius)
        glm::vec3 n1, n2, n3;    // normal (if sphere, these are not used)
        glm::vec3 t1, t2, t3;    // t.xy: texture coordinate UV (if sphere, same, not used)
    };

    const GLuint PrimitiveSize = sizeof(Primitive); // should be 12 * 11 = 132 bytes
    const GLuint BVHNodeSize = sizeof(BVHNode); // should be 4 * 4 + 3 * 2 * 4 = 40 bytes



}

#endif