#version 450 core

// IN
layout(location = 0) in vec3 inPosition;

// UNIFORM
uniform mat4 uMatShadow;

void main()
{
    gl_Position = uMatShadow * vec4(inPosition, 1.0);
}