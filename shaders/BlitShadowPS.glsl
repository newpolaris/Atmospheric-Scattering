#version 450 core

// IN
in vec2 vTexcoords;

// OUT
out vec3 FragColor;

// UNIFORM
uniform sampler2D uTexSource;

void main()
{
    float Depth = texture(uTexSource, vTexcoords).x;
    FragColor = vec3(1.0 - (1.0 - Depth)*50.0);
}