#version 450

const int NUM_CASCADES = 3;
const int MAX_POINT_LIGHTS = 2;                                                     
const int MAX_SPOT_LIGHTS = 2;                                                      

struct BaseLight
{
    vec3 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};

struct DirectionalLight                                                             
{                                                                                   
    BaseLight Base;                                                          
    vec3 Direction;                                                                 
};                                                                                  

struct PointLight
{
    BaseLight Base;
    vec3 Position;
    vec3 Attenuation;
};

struct SpotLight
{
    PointLight Base;
    vec3 Direction;
    float Cutoff;
};

// IN
in float vClipSpacePosZ;
in vec2 vTexcoords;
in vec3 vNormalWS;
in vec3 vPositionWS;
in vec4 vPositionLS[NUM_CASCADES];

// OUT
out vec4 FragColor;

uniform int uDebugType = 0;
uniform vec3 uEyePositionWS;
uniform float uSpecularPower = 0.f;
uniform float uMatSpecularIntensity = 0.f;

uniform int uNumPointLights = 0;                                                                
uniform int uNumSpotLights = 0;                                                                 
uniform float uCascadeEndClipSpace[NUM_CASCADES];
uniform DirectionalLight uDirectionalLight;                                                 
uniform PointLight uPointLights[MAX_POINT_LIGHTS];                                          
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];                                             

uniform sampler2D uTexWood;
uniform sampler2D uTexShadowmap[NUM_CASCADES];

float CalcShadowFactor(int CascadeIndex, vec4 positionLS, vec3 normal, vec3 lightDirection)
{
    vec3 ProjCoords = positionLS.xyz / positionLS.w;
    vec3 UVCoords = 0.5 * ProjCoords + 0.5;
    float Depth = texture(uTexShadowmap[CascadeIndex], UVCoords.xy).x;
    float angleBias = 0.006;
    float bias = max(angleBias * (1.0 - dot(normal, -lightDirection)), 0.0008);
    if (UVCoords.z - bias > Depth)
        return 0.5;
    return 1.0;
}

vec3 DepthPrint(vec3 normal)
{
    vec3 color = vec3(0.f);
    for (int i = 0; i < NUM_CASCADES; i++)
    {
        vec4 positionLS = vPositionLS[i];
        if (vClipSpacePosZ <= uCascadeEndClipSpace[i]) {
            vec3 ProjCoords = positionLS.xyz / positionLS.w;
            vec3 UVCoords = 0.5 * ProjCoords + 0.5;
            float Depth = texture(uTexShadowmap[i], UVCoords.xy).x;
            if (uDebugType == 1) 
                color = vec3(UVCoords.x, 0.0, UVCoords.y);
            else if (uDebugType == 2)
                color = vec3(UVCoords.z, 0.0, Depth);
            else if (UVCoords.x > 1.0 || UVCoords.x < 0.0)
                color = vec3(0, 0, 1);
            else if (UVCoords.y > 1.0 || UVCoords.y < 0.0)
                color = vec3(1, 0, 0);
            break;
        }
    }
    return color;
}

vec4 CalcLightInternal(BaseLight Light, vec3 LightDirection, vec3 Normal, float ShadowFactor)
{
    vec4 AmbientColor = vec4(Light.Color * Light.AmbientIntensity, 1.0);
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        DiffuseColor = vec4(Light.Color * Light.DiffuseIntensity * DiffuseFactor, 1.0);

        vec3 VertexToEye = normalize(uEyePositionWS - vPositionWS);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));

        float SpecularFactor = dot(VertexToEye, LightReflect);                                      
        if (SpecularFactor > 0) {                                                           
            SpecularFactor = pow(SpecularFactor, uSpecularPower);                               
            SpecularColor = vec4(Light.Color, 1.0f) * uMatSpecularIntensity * SpecularFactor;                         
        }                                                                                   
    }
    return (AmbientColor + ShadowFactor * (DiffuseColor + SpecularColor));
}

vec4 CalcDirectionalLight(DirectionalLight light, vec3 Normal, float ShadowFactor)
{
    return CalcLightInternal(light.Base, light.Direction, Normal, ShadowFactor);
}

vec4 CalcPointLight(PointLight light, vec3 normal)
{
    vec3 LightDirection = vPositionWS - light.Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);
    float ShadowFactor = 1.0;

    vec4 Color = CalcLightInternal(light.Base, LightDirection, normal, ShadowFactor);
    float Attenuation = light.Attenuation.x + light.Attenuation.y * Distance + light.Attenuation.z * Distance * Distance;
    return Color / Attenuation;
}

vec4 CalcSpotLight(SpotLight light, vec3 normal)
{
    vec3 LightToPixel = normalize(vPositionWS - light.Base.Position);
    float SpotFactor = dot(LightToPixel, light.Direction);

    if (SpotFactor > light.Cutoff) {
        vec4 Color = CalcPointLight(light.Base, normal);
        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - light.Cutoff));                   
    }
    return vec4(0, 0, 0, 0);
}
void main()
{
    vec3 normal = normalize(vNormalWS);

    float ShadowFactor = 0.0;
    vec4 CascadeIndicator = vec4(0.3, 0.0, 0.3, 0.0);

    for (int i = 0; i < NUM_CASCADES; i++)
    {
        if (vClipSpacePosZ <= uCascadeEndClipSpace[i]) {
            ShadowFactor = CalcShadowFactor(i, vPositionLS[i], normal, uDirectionalLight.Direction);
            if (i == 0)
                CascadeIndicator = vec4(0.3, 0.0, 0.0, 0.0);
            else if (i == 1)
                CascadeIndicator = vec4(0.0, 0.3, 0.0, 0.0);
            else if (i == 2)
                CascadeIndicator = vec4(0.0, 0.0, 0.3, 0.0);
            break;
        }
    }

    vec4 sumLight = CalcDirectionalLight(uDirectionalLight, normal, ShadowFactor);
    for (int i = 0; i < uNumPointLights; i++)
        sumLight += CalcPointLight(uPointLights[i], normal);
    for (int i = 0; i < uNumSpotLights; i++)
        sumLight += CalcSpotLight(uSpotLights[i], normal);

    if (uDebugType > 0)
        FragColor = vec4(DepthPrint(normal), 1.0);
    else
    {
        vec3 SampledColor = texture2D(uTexWood, vTexcoords).rgb;
        FragColor = vec4(vec3(sumLight)*SampledColor, 1.0) + CascadeIndicator;
    }
}