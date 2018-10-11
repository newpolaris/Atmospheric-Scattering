#version 450 core

#include "Common.glsli"
#include "Math.glsli"
#include "IBL/Ibl.glsli"

#define IBL_MIPMAP_LEVEL 7

// IN
in vec4 vTexcoord;
in vec3 vDirection;

// OUT
layout(location = 0) out vec4 fragColor;

uniform mat4 uMatViewInverse;
uniform sampler2D uBRDFSamp;

struct MaterialParam
{
    vec3 normal;
    vec3 albedo;
    vec3 specular;
    float smoothness;
    float alpha;
    float visibility;
    float linearDepth;
    int lightModel;
};

void ShadingMaterial(MaterialParam material, vec3 worldView, out vec3 diffuse, out vec3 sepcular)
{
    vec3 worldNormal = mat3(uMatViewInverse)*material.normal;

    vec3 V = worldView;
    vec3 N = worldNormal;
    vec3 R = EnvironmentReflect(N, V);

    float nv = abs(dot(N, V));
    float mipLayer = EnvironmentMip(IBL_MIPMAP_LEVEL - 1, pow2(material.smoothness));

    vec3 fresnel = vec3(0.0);
    fresnel = EnvironmentSpecularLUT(uBRDFSamp, nv, material.smoothness, material.specular);

    vec2 coord1 = ComputeSphereCoord(N);
    vec2 coord2 = ComputeSphereCoord(R);
}

void main()
{  
    vec2 coord = vTexcoord.xy / vTexcoord.w;

    MaterialParam material;
    MaterialParam materialAlpha;
	vec3 V = normalize(vDirection);

    vec3 diffuse = vec3(0.0), specular = vec3(0.0);
    ShadingMaterial(material, V, diffuse, specular);
    vec3 diffuse2 = vec3(0.0), specular2 = vec3(0.0);
    ShadingMaterial(materialAlpha, V, diffuse2, specular2);

    vec4 color0 = vec4(0.0);
	fragColor = color0;
}
