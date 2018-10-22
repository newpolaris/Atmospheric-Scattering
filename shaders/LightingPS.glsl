#version 450

// IN
in vec2 vTexcoords;
in vec3 vNormalWS;
in vec3 vPositionWS;
in vec4 vPositionLS;

// OUT
out vec4 FragColor;

uniform vec3 uEyePositionWS;
uniform float uSpecularPower = 0.f;
uniform float uMatSpecularIntensity = 0.f;

uniform float uAmbientIntensity;
uniform float uDiffuseIntensity;
uniform float uCutoff;
uniform vec3 uPosition;
uniform vec3 uColor;
uniform vec3 uDirection;
uniform vec3 uAttenuation;

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

uniform int gNumPointLights;                                                                
uniform int gNumSpotLights;                                                                 
uniform DirectionalLight gDirectionalLight;                                                 
uniform PointLight gPointLights[MAX_POINT_LIGHTS];                                          
uniform SpotLight gSpotLights[MAX_SPOT_LIGHTS];                                             

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

vec4 CalcDirectionalLight(DirectionalLight light, vec3 Normal)
{
    return CalcLightInternal(light.Base, light.Direction, Normal, 1.0);
}

vec4 CalcPointLight(PointLight light, vec3 normal, vec4 positionLS)
{
    vec3 LightDirection = vPositionWS - light.Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);
    float ShadowFactor = 1.0;

    vec4 Color = CalcLightInternal(light.Base, LightDirection, normal, ShadowFactor);
    float Attenuation = light.Attenuation.x + light.Attenuation.y * Distance + light.Attenuation.z * Distance * Distance;
    return Color / Attenuation;
}

vec4 CalcSpotLight(SpotLight light, vec3 normal, vec4 positionLS)
{
    vec3 LightToPixel = normalize(vPositionWS - light.Base.Position);
    float SpotFactor = dot(LightToPixel, uDirection);

    if (SpotFactor > light.Cutoff) {
        vec4 Color = CalcPointLight(light.Base, normal, positionLS);
        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - light.Cutoff));                   
    }
    return vec4(0, 0, 0, 0);
}

void main()
{
    SpotLight light;
    light.Base.Base.Color = uColor;
    light.Base.Base.AmbientIntensity = uAmbientIntensity;
    light.Base.Base.DiffuseIntensity = uDiffuseIntensity;
    light.Base.Position = uPosition;
    light.Base.Attenuation = uAttenuation;
    light.Direction = uDirection;
    light.Cutoff = uCutoff;

    vec3 normal = normalize(vNormalWS);
    vec4 sumLight = CalcSpotLight(light, normal, vPositionLS);

    FragColor = sumLight;
}