#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    //remove translation from view matrix
    mat3 rotationMatrix = mat3(view); // 移除位移，保留旋转和缩放

    TexCoords = aPos;
    vec4 pos = projection * mat4(rotationMatrix) * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  