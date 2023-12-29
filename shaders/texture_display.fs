#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;

void main()
{
    vec2 flippedCoords = vec2(TexCoords.x, 1.0 - TexCoords.y);
    FragColor = texture(texture1, flippedCoords);
}