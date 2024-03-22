#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 rotation;
uniform mat4 model;

layout(std140, binding = 0) uniform RenderInfo{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    float refractionRatio;
    float averageSlope;
    int renderMode;
    vec3 ambientK;
    vec3 diffuseK;
    vec3 specularK;
};

out vec3 Normal;
out vec3 FragPos;

void main()
{
    mat4 finalModel = model * rotation;
    gl_Position = projection * view * finalModel * vec4(aPos, 1.0f);
    Normal = mat3(transpose(inverse(finalModel))) * aNormal;
    FragPos = vec3(finalModel * vec4(aPos, 1.0f));
}
