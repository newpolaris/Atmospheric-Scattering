#version 450

const int NUM_CASCADES = 4;
const int MAX_POINT_LIGHTS = 2;                                                     
const int MAX_SPOT_LIGHTS = 2;                                                      

struct DirectionalLight                                                             
{   
    vec3 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};                                                                                  

// IN
in float vClipSpacePosZ;
in vec2 vTexcoords;
in vec3 vNormalWS;
in vec3 vViewdirWS;
in vec3 vPositionWS;
in vec4 vPositionLS[NUM_CASCADES];

// OUT
out vec4 FragColor;

uniform vec3 uEyePositionWS;
uniform float uSpecularPower = 0.f;
uniform float uMatSpecularIntensity = 0.f;
uniform vec3 uLightDirection;
uniform float uCascadeEndClipSpace[NUM_CASCADES];
uniform DirectionalLight uDirectionalLight;                                                 

uniform sampler2D uTexShadowmap[NUM_CASCADES];
uniform sampler2D uTexDiffuseMapSamp;
uniform sampler2D uTexNormalMapSamp;
uniform sampler2D uTexDepthMapSamp;

float CalcShadowFactor(int CascadeIndex, vec4 positionLS, vec3 normal, vec3 lightDirection)
{
    const float angleBias = 0.006;

    vec3 ProjCoords = positionLS.xyz / positionLS.w;
    vec3 UVCoords = 0.5 * ProjCoords + 0.5;
    float Depth = texture(uTexShadowmap[CascadeIndex], UVCoords.xy).x;
    float bias = max(angleBias * (1.0 - dot(normal, -lightDirection)), 0.0008);
    if (UVCoords.z - bias > Depth)
        return 0.5;
    return 1.0;
}

vec3 CalcDirectionalLight(DirectionalLight light, vec3 cameradireciton, vec3 Direction, vec3 Normal, float ShadowFactor)
{
    vec3 AmbientColor = vec3(light.Color * light.AmbientIntensity);
    float DiffuseFactor = dot(Normal, -Direction);

    vec3 DiffuseColor = vec3(0, 0, 0);
    vec3 SpecularColor = vec3(0, 0, 0);

    if (DiffuseFactor > 0) {
        DiffuseColor = vec3(light.Color * light.DiffuseIntensity * DiffuseFactor);

        vec3 LightReflect = normalize(reflect(Direction, Normal));
        float SpecularFactor = dot(cameradireciton, LightReflect);                                      
        if (SpecularFactor > 0) {                                                           
            SpecularFactor = pow(SpecularFactor, uSpecularPower);                               
            SpecularColor = vec3(light.Color) * uMatSpecularIntensity * SpecularFactor;                         
        }                                                                                   
    }
    return (AmbientColor + ShadowFactor * (DiffuseColor + SpecularColor));
}

// method used in Ray-MMD
mat3x3 CalcBumpedNormal(vec3 normal)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(vViewdirWS);
    vec3 dp2 = dFdy(vViewdirWS);
    vec3 duv1 = dFdx(vec3(vTexcoords, 0.0));
    vec3 duv2 = dFdy(vec3(vTexcoords, 0.0));

    // solve the linear system
    vec3 dp2perp = cross(dp2, normal);
    vec3 dp1perp = cross(normal, dp1);
    mat3x3 M = mat3x3(dp1, dp2, normal);
    mat2x3 I = mat2x3(dp2perp, dp1perp);
    vec3 T = I*vec2(duv1.x, duv2.x);
    vec3 B = I*vec2(duv1.y, duv2.y);

    // construct a scale-invariant frame
    mat3x3 tbnTransform;
    float scaleT = 1.0 / (dot(T, T) + 1e-6);
    float scaleB = 1.0 / (dot(B, B) + 1e-6);

    tbnTransform[0] = normalize(T * scaleT);
    tbnTransform[1] = normalize(B * scaleB);
    tbnTransform[2] = normal;

    return tbnTransform;
}

void main()
{
    mat3 tbnTransform = transpose(CalcBumpedNormal(normalize(vNormalWS)));
    vec3 normal = texture2D(uTexNormalMapSamp, vTexcoords).rgb;
    vec3 normalTS = tbnTransform*normal;
    vec3 lightdirectionTS = tbnTransform*uLightDirection;
    vec3 positionTS = tbnTransform*vPositionWS;
    vec3 cameraPositionTS = tbnTransform*uEyePositionWS;
    vec3 cameradirectionTS = normalize(cameraPositionTS - positionTS);

    float ShadowFactor = 0.0;
    vec4 CascadeIndicator = vec4(0.3, 0.0, 0.3, 0.0);

    for (int i = 0; i < NUM_CASCADES; i++)
    {
        if (vClipSpacePosZ <= uCascadeEndClipSpace[i]) {
            ShadowFactor = CalcShadowFactor(i, vPositionLS[i], normalTS, lightdirectionTS);
            if (i == 0)
                CascadeIndicator = vec4(0.3, 0.0, 0.0, 0.0);
            else if (i == 1)
                CascadeIndicator = vec4(0.0, 0.3, 0.0, 0.0);
            else if (i == 2)
                CascadeIndicator = vec4(0.0, 0.0, 0.3, 0.0);
            break;
        }
    }

    vec3 sumLight = CalcDirectionalLight(uDirectionalLight, cameradirectionTS, lightdirectionTS, normalTS, ShadowFactor);
    vec3 SampledColor = texture2D(uTexDiffuseMapSamp, vTexcoords).rgb;
    FragColor = vec4(vec3(sumLight)*SampledColor, 1.0); // + CascadeIndicator;
}