#version 450 core

// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoords;

// Out
out vec2 vTexcoords;

void main()
{
    vTexcoords = inTexcoords;
    gl_Position = vec4(inPosition, 1.0);
}