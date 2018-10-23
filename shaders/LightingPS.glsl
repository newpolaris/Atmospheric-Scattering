#version 450

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
in vec2 vTexcoords;
in vec3 vNormalWS;
in vec3 vPositionWS;
in vec4 vPositionLS;

// OUT
out vec4 FragColor;

uniform vec3 uEyePositionWS;
uniform float uSpecularPower = 0.f;
uniform float uMatSpecularIntensity = 0.f;

uniform int uNumPointLights;                                                                
uniform int uNumSpotLights;                                                                 
uniform DirectionalLight uDirectionalLight;                                                 
uniform PointLight uPointLights[MAX_POINT_LIGHTS];                                          
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];                                             

uniform sampler2D uTexShadowmap;

float CalcShadowFactor(vec4 positionLS)
{
    vec3 ProjCoords = positionLS.xyz / positionLS.w;
    vec3 UVCoords = 0.5 * ProjCoords + 0.5;
    float Depth = texture(uTexShadowmap, UVCoords.xy).x;
    if (Depth < UVCoords.z + 1e-4f)
        return 1.0;
    return 1.0;
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

vec4 CalcDirectionalLight(DirectionalLight light, vec3 Normal, vec4 LightSpacePos)
{
    float ShadowFactor = CalcShadowFactor(LightSpacePos);
    return CalcLightInternal(light.Base, light.Direction, Normal, ShadowFactor);
}

vec4 CalcPointLight(PointLight light, vec3 normal, vec4 positionLS)
{
    vec3 LightDirection = vPositionWS - light.Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);
    float ShadowFactor = CalcShadowFactor(positionLS);

    vec4 Color = CalcLightInternal(light.Base, LightDirection, normal, ShadowFactor);
    float Attenuation = light.Attenuation.x + light.Attenuation.y * Distance + light.Attenuation.z * Distance * Distance;
    return Color / Attenuation;
}

vec4 CalcSpotLight(SpotLight light, vec3 normal, vec4 positionLS)
{
    vec3 LightToPixel = normalize(vPositionWS - light.Base.Position);
    float SpotFactor = dot(LightToPixel, light.Direction);

    if (SpotFactor > light.Cutoff) {
        vec4 Color = CalcPointLight(light.Base, normal, positionLS);
        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - light.Cutoff));                   
    }
    return vec4(0, 0, 0, 0);
}

void main()
{
    vec3 normal = normalize(vNormalWS);
    vec4 sumLight = CalcDirectionalLight(uDirectionalLight, normal, vPositionLS);
    for (int i = 0; i < uNumPointLights; i++)
        sumLight += CalcPointLight(uPointLights[i], normal, vPositionLS);
    for (int i = 0; i < uNumSpotLights; i++)
        sumLight += CalcSpotLight(uSpotLights[i], normal, vPositionLS);
    FragColor = sumLight;
}