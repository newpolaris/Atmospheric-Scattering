--Vertex

#define VERTEX_SHADER 1
#include "Common.glsli"
#include "Math.glsli"

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

// OUT
out vec4 vTexcoords;
out vec3 vViewdir;

// UNIFORM
uniform vec3 uCameraPosition;
uniform mat4 uModelToProj;

void main()
{
    float scale = 1000.0;
    vec4 position = vec4(inPosition.xyz * scale, 1.0);
    gl_Position = vTexcoords = uModelToProj * position;
    vTexcoords.xy = PosToCoord(vTexcoords.xy / vTexcoords.w);
    vTexcoords.xy = vTexcoords.xy * vTexcoords.w;
    vViewdir = uCameraPosition - position.xyz;
}

--

//------------------------------------------------------------------------------


--Fragment

// IN
in vec4 vTexcoords;
in vec3 vViewdir;

// OUT
layout(location = 0) out vec4 fragColor;

// UNIFORM
uniform sampler2D uBuffer1;
uniform sampler2D uBuffer2;
uniform sampler2D uBuffer3;
uniform sampler2D uBuffer4;
uniform samplerCube uEnvmapIrr;
uniform samplerCube uEnvmapPrefilter;
uniform sampler2D uEnvmapBrdfLUT;

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
uniform mat4 uView;
uniform mat4 uInverseView;
uniform mat4 uInverseProj;
uniform mat4 uInverseViewProj;

const float pi = 3.14159265359;

vec3 calcFresnel(vec3 _cspec, float _dot, float _strength)
{
    return _cspec + (1.0 - _cspec)*pow(1.0 - _dot, 5.0) * _strength;
}

vec3 calcLambert(vec3 _cdiff, float _ndotl)
{
    return _cdiff*_ndotl;
}

vec3 calcBlinn(vec3 _cspec, float _ndoth, float _ndotl, float _specPwr)
{
    float norm = (_specPwr + 8.0)*0.125;
    float brdf = pow(_ndoth, _specPwr)*_ndotl*norm;
    return _cspec*brdf;
}

float specPwr(float _gloss)
{
    return exp2(10.0*_gloss + 2.0);
}

float random(vec2 _uv)
{
    return fract(sin(dot(_uv.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 fixCubeLookup(vec3 _v, float _lod, float _topLevelCubeSize)
{
    // Reference:
    // Seamless cube-map filtering
    // http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
    float ax = abs(_v.x);
    float ay = abs(_v.y);
    float az = abs(_v.z);
    float vmax = max(max(ax, ay), az);
    float scale = 1.0 - exp2(_lod) / _topLevelCubeSize;
    if (ax != vmax) { _v.x *= scale; }
    if (ay != vmax) { _v.y *= scale; }
    if (az != vmax) { _v.z *= scale; }
    return _v;
}

// use code from learnOpenGL
float distributionGGX(float _ndoth, float roughness)
{
    // due to alpha = roughness^2
    float a = roughness*roughness;
    float a2 = a*a;
    float denom = _ndoth*_ndoth * (a2 - 1.0) + 1.0;
    denom = max(0.001, denom);
    return a2 / (pi * denom * denom);
}

float geometrySchlickGGX(float _ndotv, float roughness)
{
    float r = roughness + 1.0;
    float k = r*r / 8.0;
    float nom = _ndotv;
    float denom = _ndotv * (1.0 - k) + k;
    return nom / denom;
}

float geometrySmith(float _ndotv, float _ndotl, float roughness)
{
    float ggx2 = geometrySchlickGGX(_ndotv, roughness);
    float ggx1 = geometrySchlickGGX(_ndotl, roughness);
    return ggx1 * ggx2;
}

mat3 calcTbn(vec3 _normal, vec3 _worldPos, vec2 _texCoords)
{
    vec3 Q1 = dFdx(_worldPos);
    vec3 Q2 = dFdy(_worldPos);
    vec2 st1 = dFdx(_texCoords);
    vec2 st2 = dFdy(_texCoords);

    vec3 N = _normal;
    vec3 T = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B = -normalize(cross(N, T));
    return mat3(T, B, N);
}

// Moving frostbite to pbr
vec3 getSpecularDomninantDir(vec3 N, vec3 R, float roughness)
{
    float smoothness = 1 - roughness;
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    // The result is not normalized as we fetch in a cubemap
    return mix(N, R, lerpFactor);
}

vec3 ReconstructWorldPositionFromDepth(vec2 coord, float depth)
{
    vec4 projectedPosition = vec4(coord * 2 - 1.0, depth, 1.0);
    vec4 position = uInverseViewProj * projectedPosition;
    return position.xyz / position.w;
}

void main()
{
    vec2 coords = vTexcoords.xy / vTexcoords.w;

    vec4 buffer1 = texture(uBuffer1, coords);
    vec4 buffer2 = texture(uBuffer2, coords);
    vec4 buffer3 = texture(uBuffer3, coords);
    vec4 buffer4 = texture(uBuffer4, coords);

    vec3 V = normalize(vViewdir);

    // Material params.
    vec3 inAlbedo = buffer1.xyz;
    float inMetallic = buffer1.w;
    float linearDepth1 = buffer2.x;
    float inRoughness = buffer2.w;
    vec3 vWorldPosWS = buffer3.xyz;
    vec3 vNormalWS = buffer4.xyz;
    float depth = buffer3.w;

    vec3 vViewDirWS = V;

#define DEPTH_VIEW 3
#if DEPTH_VIEW == 1
    vec3 posWS = ReconstructWorldPositionFromDepth(coords, depth);
    fragColor = vec4(vec3(length(posWS - vWorldPosWS)), 1.0);
    return;
#elif DEPTH_VIEW == 2
    float scaler = 10.0; // too small to view
    fragColor = vec4(vec3(linearDepth1 * scaler), 1.0);
    return;
#elif DEPTH_VIEW == 3
    vec2 ndc = coords*2.0 - 1.0;
    vec4 ray = uInverseProj * vec4(ndc, 1.0, 1.0);
    float far = 10000.0;
    vec3 posVS = ray.xyz * -linearDepth1 * far;
    vec3 refVS = vec3(uView*vec4(vWorldPosWS, 1.0));
    fragColor = vec4(vec3(length(posVS - refVS)), 1.0);
    // fragColor = vec4(vec3(length(refVS.z - linearDepth1)), 1.0);
    // fragColor = vec4(vec3(length(refVS.z - posVS.z)), 1.0);
    // fragColor = vec4(vec3(length(ray.z + 1)), 1.0);
    return;
#elif DEPTH_VIEW == 5
    vec2 ndc = coords*2.0 - 1.0;
    vec4 ray = uInverseProj * vec4(ndc, 1.0, 1.0);
    float far = 10000.0;
    vec3 posVS = ray.xyz * far * linearDepth1;
    vec3 refVS = vec3(uView*vec4(vWorldPosWS, 1.0));
    fragColor = vec4(vec3(length(posVS - refVS)), 1.0);
    return;
#else
    vec2 ndc = coords*2.0 - 1.0;
    vec4 ray = uInverseProj * vec4(ndc, 1.0, 1.0);
    float far = 10000.0;
    vec3 posVS = ray.xyz * far * linearDepth1;
    vec3 posWS = vec3(uInverseView * vec4(posVS, 1.0));
    fragColor = vec4(vec3(length(posWS.z - vWorldPosWS.z)), 1.0);
    return;
#endif

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 f0 = mix(vec3(0.04), inAlbedo, inMetallic);

    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    vec3 albedo = inAlbedo * (1.0 - inMetallic);

    // Input.
    vec3 nn = normalize(vNormalWS);
    vec3 vv = normalize(vViewDirWS);

    // reflectance equation
    vec3 direct = vec3(0.0);
    for (int i = 0; i < 4; ++i)
    {
        // calculate per-light radiance
        vec3 ld = normalize(uLightPositions[i] - vWorldPosWS);
        vec3 hh = normalize(vv + ld);
        float distance = length(uLightPositions[i] - vWorldPosWS);
        float attenuation = 1.0 / (distance*distance);
        vec3 radiance = uLightColors[i] * attenuation;

        float ndotv = clamp(dot(nn, vv), 0.0, 1.0);
        float ndotl = clamp(dot(nn, ld), 0.0, 1.0);
        float ndoth = clamp(dot(nn, hh), 0.0, 1.0);
        float hdotv = clamp(dot(hh, vv), 0.0, 1.0);

        float d = distributionGGX(ndoth, inRoughness);
        float g = geometrySmith(ndotv, ndotl, inRoughness);
        vec3 f = calcFresnel(f0, hdotv, 1.0);

        vec3 nominator = d * g * f;
        // prevent divide by zero
        float denominator = max(0.001, 4 * ndotv * ndotl);
        vec3 specular = nominator / denominator * ubSpecular;

        // kS is equal to Fresnel
        vec3 kS = f;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;

        // scale light by NdotL
        vec3 diffuse = kD * albedo / pi * ubDiffuse;
        direct += (diffuse + specular)*radiance*ndotl;
    }

    float ndotv = clamp(dot(nn, vv), 0.0, 1.0);
    vec3 envFresnel = calcFresnel(f0, ndotv, 1);
    vec3 r = 2.0 * nn * ndotv - vv; // =reflect(-toCamera, normal) 
    r = getSpecularDomninantDir(nn, r, inRoughness);
    vec3 kS = envFresnel;
    vec3 kD = 1.0 - envFresnel;
    vec3 irradiance = texture(uEnvmapIrr, nn).xyz;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(uEnvmapPrefilter, r, inRoughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(uEnvmapBrdfLUT, vec2(ndotv, inRoughness)).rg;
    vec3 radiance = prefilteredColor * (kS * brdf.x + brdf.y);
    vec3 envDiffuse = albedo*kD  * irradiance * ubDiffuseIbl;
    vec3 envSpecular = radiance   * ubSpecularIbl;
    vec3 indirect = envDiffuse + envSpecular;

    // Color.
    vec3 color = direct + indirect;
    color = color * exp2(uExposure);
    fragColor = vec4(color, 1.0);
}

