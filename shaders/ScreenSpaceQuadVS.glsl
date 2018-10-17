#version 450 core

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec2 coord;
out vec3 viewdir;

uniform mat4 uProjectInverse;

void main()
{
	coord = inTexcoords;
    viewdir = -vec3(uProjectInverse * vec4(inPosition, 1.0));
	gl_Position = vec4(inPosition, 1.0);
}
