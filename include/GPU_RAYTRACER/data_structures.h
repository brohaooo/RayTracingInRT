#ifndef GPU_DATA_STRUCTURES_H
#define GPU_DATA_STRUCTURES_H

#include <glm/glm.hpp>

namespace GPU_RAYTRACER{

    struct Material {
        int type; // 0: lambertian, 1: metal, 2: dielectric
        float fuzz; // fuzziness of the metal material
        glm::vec3 baseColor;
    };
    // triangle mesh
    struct Triangle {
        glm::vec3 v0, v1, v2;    // position
        glm::vec3 n1, n2, n3;    // normal
        glm::vec2 t1, t2, t3;    // texture coordinate UV
        Material material;       // material (per triangle)
    };
    // sphere (individual, not in a mesh) // currently not used
    struct Sphere {
        glm::vec3 center;        // center position
        float radius;            // radius
        Material material;       // material (per sphere)
    };

    // BVH tree node
    struct BVHNode {
        int left, right;         // left and right child index
        int n, index;            // number of triangles in the node, and the start index of the triangles in this node               
        glm::vec3 AA, BB;        // its AABB bounding box
    };



}

#endif