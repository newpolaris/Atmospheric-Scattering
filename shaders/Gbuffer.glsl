--Vertex

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoords;

// OUT
out vec3 vNormalWS;
out vec3 vViewDirWS;
out vec3 vWorldPosWS;
out vec3 vPositionVS;
out vec2 vTexcoords;
out vec2 vDepthZW;

// UNIFORM
uniform mat4 uMtxSrt;
uniform mat4 uModelViewMatrix;
uniform mat4 uModelViewProjMatrix;
uniform vec3 uEyePosWS;

void main()
{
    // Clip Space position
    gl_Position = uModelViewProjMatrix * uMtxSrt * inPosition;
    vec4 positionVS = uModelViewMatrix * uMtxSrt * inPosition;

    // World Space normal
    vec3 normal = mat3(uMtxSrt) * inNormal;
    vNormalWS = normalize(normal);

    vTexcoords = inTexcoords;

    // World Space view direction from world space position
    vec3 posWS = vec3((uMtxSrt * inPosition).xyz);
    vViewDirWS = normalize(uEyePosWS - posWS);
    vWorldPosWS = posWS;
    vDepthZW = gl_Position.zw;
    vPositionVS = positionVS.xyz;
}


--

//------------------------------------------------------------------------------


--Fragment

// IN
in vec3 vNormalWS;
in vec3 vViewDirWS;
in vec3 vWorldPosWS;
in vec3 vPositionVS;
in vec2 vTexcoords;
in vec2 vDepthZW;

// OUT
layout(location = 0) out vec4 Gbuffer1RT;
layout(location = 1) out vec4 Gbuffer2RT;
layout(location = 2) out vec4 Gbuffer3RT;
layout(location = 3) out vec4 Gbuffer4RT;

#include "Common.glsli"
#include "Math.glsli"
#include "EncodeNormal.glsli"
#include "Gbuffer.glsli"
#include "LinearDepth.glsli"

uniform float ubMetalOrSpec;
uniform float ubDiffuse;
uniform float ubSpecular;
uniform float ubDiffuseIbl;
uniform float ubSpecularIbl;
uniform float uGlossiness;
uniform float uReflectivity;
uniform float uExposure;
uniform vec3 uLightDir;
uniform vec3 uLightCol;
uniform vec3 uRgbDiff;
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];
uniform vec3 uEyePosWS;
uniform mat4 uProjection;

const float A = uProjection[2].z;
const float B = uProjection[3].z;
const float near = -B / (1.0 - A);
const float far = B / (1.0 + A);


struct MaterialParam
{
    vec3 normal;
    vec3 albedo;
    vec3 specular;
	vec3 emissive;
	float smoothness;
	float metalness;
	float emissiveIntensity;
	float alpha;
	float visibility;
	float customDataA;
	vec3 customDataB;
	int lightModel;

    float distance;
    float roughness;
    float linearDepth;
};

struct GbufferParam
{
    vec4 buffer1;
    vec4 buffer2;
    vec4 buffer3;
    vec4 buffer4;
};

// linearDepth: -positionVS.z
GbufferParam EncodeGbuffer(MaterialParam material, float linearDepth)
{
    GbufferParam gbuffer;
    gbuffer.buffer1 = vec4(material.albedo, material.metalness);
    gbuffer.buffer2 = vec4(material.linearDepth, material.distance, 0.0, material.roughness);
    gbuffer.buffer3 = vec4(EncodeNormal(material.normal), 0.0);
    return gbuffer;
}

void main()
{
    MaterialParam material;
    material.albedo = uRgbDiff;
    material.normal = normalize(vNormalWS);
    material.smoothness = 0.0;
    material.metalness = uReflectivity;
    material.roughness = uGlossiness;
    material.linearDepth = LinearizeDepth(gl_FragCoord.z, near, far);
    material.distance = length(vWorldPosWS - uEyePosWS);
    GbufferParam gbuffer = EncodeGbuffer(material, -vPositionVS.z);
    Gbuffer1RT = gbuffer.buffer1;
    Gbuffer2RT = gbuffer.buffer2;
    Gbuffer3RT = gbuffer.buffer3;
    Gbuffer4RT = gbuffer.buffer4;
}