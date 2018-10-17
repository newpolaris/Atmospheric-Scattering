#version 450 core

#include "System.conf"
#include "Common.glsli"
#include "Math.glsli"
#include "Gbuffer.glsli"
#include "EncodeNormal.glsli"
#include "IBL/Ibl.glsli"

// IN
in vec2 coord;
in vec3 viewdir;

// OUT
layout(location = 0) out vec4 oColor0;
layout(location = 1) out vec4 oColor1;

uniform float uSunShadowVM = 1.0;
uniform vec2 uViewportSize;
uniform vec3 uSunDirection;
uniform mat4 uMatView;
uniform sampler2D uEnvLightMapSamp;
uniform sampler2D uGbuffer1Map;
uniform sampler2D uGbuffer2Map;
uniform sampler2D uGbuffer3Map;
uniform sampler2D uGbuffer4Map;

const vec4 screenPosition = gl_FragCoord;
const vec2 ViewportOffset = 0.5 / uViewportSize;
const vec2 ViewportOffset2 = 1.0 / uViewportSize;
const float ViewportAspect = uViewportSize.x / uViewportSize.y;

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
};

void DecodeGbuffer(vec4 buffer1, vec4 buffer2, vec4 buffer3, vec4 buffer4, out MaterialParam material)
{
    material.albedo = buffer1.xyz;
    material.specular = vec3(0.0);

    material.normal = DecodeNormal(buffer3.xyz);
    material.smoothness =  1 - sqrt(buffer2.w);
    material.alpha = 1.0;
    material.visibility = 1.0;
}

void ShadingImageBasedLighting(sampler2D source, MaterialParam material, vec4 screenPosition, vec2 coord, vec3 V, vec3 L, out vec3 diffuse, out vec3 specular)
{
#if IBL_QUALITY
    DecodeYcbcr(source, coord, screenPosition, ViewportOffset2, diffuse, specular);

#if SSDO_QUALITY
#else
    diffuse *= material.visibility;
#endif

    float shadow = 1.0;
    float envShadow = uSunShadowVM;

    diffuse *= shadow;
    specular *= (shadow * shadow * shadow);
#endif
}

void main()
{
    vec4 MRT1 = textureLod(uGbuffer1Map, coord, 0);
    vec4 MRT2 = textureLod(uGbuffer2Map, coord, 0);
    vec4 MRT3 = textureLod(uGbuffer3Map, coord, 0);
    vec4 MRT4 = textureLod(uGbuffer4Map, coord, 0);

    MaterialParam material;
    DecodeGbuffer(MRT1, MRT2, MRT3, MRT4, material);

    vec3 V = normalize(viewdir);
    vec3 L = -mat3(uMatView) * uSunDirection;

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

#if SUN_LIGHT_ENABLE
    vec3 sunDiffuse, sunSpecular;
    ShadingMaterial(material, V, L, coord, sunDiffuse, sunSpecular);
    diffuse += sunDiffuse;
    specular += sunSpecular;
#endif

#if 1 
    vec3 iblDiffuse, iblSpecular;
    ShadingImageBasedLighting(uEnvLightMapSamp, material, screenPosition, coord, V, L, iblDiffuse, iblSpecular);
    diffuse += iblDiffuse;
    specular += iblSpecular;
#endif

    oColor0 = vec4(specular, 1.0);
    oColor1 = vec4(max(vec3(0.0), V), 1.0);
}
