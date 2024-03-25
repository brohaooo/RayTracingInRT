#ifndef GPU_RAYTRACER_RAYTRACE_MANAGER_H
#define GPU_RAYTRACER_RAYTRACE_MANAGER_H

#include "../Object.h"
#include "data_structures.h"
#include <vector>

namespace GPU_RAYTRACER{

    class RaytraceManager{
    public:
        RaytraceManager();
        ~RaytraceManager();
        // include skybox, objects, dynamic_cast to check the type of the object
        // and store the object in the corresponding list (triangles, spheres, skybox)
        void loadScene(std::vector<Object*>);
        // with its gpu ray tracing shader, render the scene (two passes: first pass for ray tracing, second pass for displaying the result)
        // the first pass: it will write the result to the 'renderTexture'
        void compute();
        // construct BVH tree for the scene (in CPU, then upload to GPU)
        void constructBVH();
        // upload scene data to gpu: bvh nodes, objects(geometries(triangles/spheres), materials)
        void uploadSceneData();
        // update BVH tree, and upload it again to the GPU
        void updateBVH();
        // set shaders for the ray tracing pipeline: compute shader for ray tracing, vertex/fragment shaders for displaying the result
        // hard coded shaders for now, since we are not going to change them
        void setShaders();
        // activate the GPU ray tracing pipeline, it will bind the shader for screenCanvas, so that
        // visualizing the result of the ray tracing will be possible from outside by letting the screenCanvas to render
        void activate();
        // deactivate the GPU ray tracing pipeline, it will unbind the shader for screenCanvas
        void deactivate();
    private:
        GLuint bvhBuffer; // ssbo for bvh nodes
        GLuint primitiveBuffer; // ssbo for primitives (triangles/spheres) !!! it might be too slow in SSBO, consider using texture buffer
        bool hasSkybox = false;
        GLuint skyboxTexture; // skybox texture
        GLuint renderTexture; // texture to render the result of the ray tracing
        std::vector<Triangle> triangles;
        std::vector<Sphere> spheres;
        std::vector<BVHNode> bvhNodes;
        // shaders
        GLuint raytraceComputeShader;
        GLuint displayVertexShader;
        GLuint displayFragmentShader;
        // ref to screen quad object 
        Rect * screenCanvas;
        // ref to camera object, to get the view matrix and the projection matrix
        // to figure the ray directions in the compute shader
        Camera * camera;
        



    };















}


#endif 