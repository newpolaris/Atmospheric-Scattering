#version 450

// IN
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoords;

// OUT
out vec2 vTexcoords;
out vec3 vNormalWS;
out vec3 vPositionWS;
out vec4 vPositionLS;

// UNIFORM
uniform mat4 uMatWorld;
uniform mat4 uMatWorldViewProject;
uniform mat4 uMatLight;

void main()
{
    vec4 position = vec4(inPosition, 1.0);
    vTexcoords = inTexcoords;
    vNormalWS = mat3(uMatWorld) * inNormal;
    vPositionWS = vec3(uMatWorld * position);
    vPositionLS = uMatLight * vec4(vPositionWS, 1.0);
    gl_Position = uMatWorldViewProject * position;
}
