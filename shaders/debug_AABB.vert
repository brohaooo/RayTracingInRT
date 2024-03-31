#version 430 core


layout(std140, binding = 0) uniform RenderInfo{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
};

layout (location = 0) in vec3 aPos;

uniform vec3 AABB_min; // AABB的最小顶点
uniform vec3 AABB_max; // AABB的最大顶点

void main()
{
    // 将顶点位置从[-1, 1]范围映射到[AABB_min, AABB_max]范围
    vec3 position = 0.5 * (aPos + 1.0) * (AABB_max - AABB_min) + AABB_min;
    gl_Position = projection * view * vec4(position, 1.0);
}
