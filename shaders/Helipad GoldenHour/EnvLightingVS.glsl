#version 450 core

#define VERTEX_SHADER 1
#include "../Common.glsli"
#include "../Math.glsli"

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

// OUT
out vec4 vTexcoord;
out vec3 vDirection;

// UNIFORM
uniform vec3 uCameraPosition;
uniform mat4 uModelToProj;

void main()
{
    vec4 position = vec4(inPosition.xyz, 1.0);
    gl_Position = vTexcoord = uModelToProj * position;
    vTexcoord.xy = PosToCoord(vTexcoord.xy / vTexcoord.w);
    vTexcoord.xy = vTexcoord.xy * vTexcoord.w;
    vDirection = uCameraPosition - position.xyz;
}
