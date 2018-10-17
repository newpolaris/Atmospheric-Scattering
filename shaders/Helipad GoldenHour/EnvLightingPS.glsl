#version 450 core

#include "../Common.glsli"
#include "../Math.glsli"
#include "../Gbuffer.glsli"
#include "../EncodeNormal.glsli"
#include "../IBL/Ibl.glsli"

#define IBL_MIPMAP_LEVEL 7

// IN
in vec4 vTexcoord;
in vec3 vDirection;

// OUT
layout(location = 0) out vec4 oColor0;
layout(location = 1) out vec4 oColor1;

uniform mat4 uMatViewInverse;
uniform float uEnvIntensityDiff = 1.0;
uniform float uEnvIntensitySpec = 1.0;
uniform vec4 uBalanceDiffuse = vec4(0.0);

uniform sampler2D uBRDFSamp;
uniform sampler2D uDiffuseSamp;
uniform sampler2D uSpecularSamp;
uniform sampler2D uGbuffer1;
uniform sampler2D uGbuffer2;
uniform sampler2D uGbuffer3;
uniform sampler2D uGbuffer4;
uniform sampler2D uGbuffer5;
uniform sampler2D uGbuffer6;
uniform sampler2D uGbuffer7;
uniform sampler2D uGbuffer8;

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

void DecodeGbuffer(vec4 buffer1, vec4 buffer2, vec4 buffer3, vec4 buffer4, out MaterialParam material)
{
    material.albedo = buffer1.xyz;
    material.specular = vec3(0.0);

    material.normal = DecodeNormal(buffer3.xyz);
    material.smoothness = 0.0;

    material.alpha = 1.0;

    material.linearDepth = buffer2.x;
    material.visibility = 0.0;
    material.lightModel = 1;
}

void ShadingMaterial(MaterialParam material, vec3 worldView, out vec3 diffuse, out vec3 specular)
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
    
    vec3 prefilteredDiffuse = DecodeRGBT(textureLod(uDiffuseSamp, coord1, 0));
    vec3 prefilteredSpecular = DecodeRGBT(textureLod(uSpecularSamp, coord2, mipLayer));
    prefilteredDiffuse = ColorBalance(prefilteredDiffuse, uBalanceDiffuse);

    diffuse = prefilteredDiffuse * uEnvIntensityDiff;
    specular = prefilteredSpecular; // * fresnel;

    specular *= uEnvIntensitySpec;
}

void main()
{  
    vec4 screenPosition = gl_FragCoord;
    vec2 coords = vTexcoord.xy / vTexcoord.w;
    vec4 MRT1 = texture(uGbuffer1, coords);
    vec4 MRT2 = texture(uGbuffer2, coords);
    vec4 MRT3 = texture(uGbuffer3, coords);
    vec4 MRT4 = texture(uGbuffer4, coords);

    MaterialParam material;
    DecodeGbuffer(MRT1, MRT2, MRT3, MRT4, material);

	vec3 V = normalize(vDirection);

    vec3 diffuse, specular;
    ShadingMaterial(material, V, diffuse, specular);
    oColor0 = EncodeYcbcr(screenPosition, diffuse, specular);
    oColor1 = EncodeYcbcr(screenPosition, diffuse, specular);
}
