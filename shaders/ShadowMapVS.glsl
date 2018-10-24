#version 450 core

// IN
layout(location = 0) in vec3 inPosition;

// UNIFORM
uniform mat4 uMatModel;
uniform mat4 uMatLightSpace;

void main()
{
    gl_Position = uMatLightSpace * uMatModel * vec4(inPosition, 1.0);
}