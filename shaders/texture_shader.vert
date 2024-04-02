#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;

layout(std140, binding = 0) uniform RenderInfo{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
};

out vec2 TexCoords;
out vec3 Normal;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
    TexCoords = vec2(aTexCoord.x, aTexCoord.y);
    Normal = mat3(transpose(inverse(model))) * aNormal;
}