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
uniform mat4 uMatModel;
uniform mat4 uMatView;
uniform mat4 uMatProject;
uniform mat4 uMatLight;

void main()
{
    vec4 position = vec4(inPosition, 1.0);
    vTexcoords = inTexcoords;
    vNormalWS = transpose(inverse(mat3(uMatModel))) * inNormal;
    vPositionWS = vec3(uMatModel * position);
    vPositionLS = uMatLight * uMatModel * position;
    gl_Position = uMatProject * uMatView * uMatModel * position;
}
