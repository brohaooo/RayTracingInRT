#version 430 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Normal; // not used currently in this shader

uniform sampler2D texture1;
uniform vec4 color;



void main()
{
    FragColor = texture(texture1, TexCoords) * color;
}
