#pragma once

// TEST OLD STYLE SHADER MANAGEMENT

#include <stdint.h>
#include <GL/glew.h>
#include <GLType/ProgramShader.h>

struct BaseLight
{
    std::string Name;
    glm::vec3 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};

struct DirectionalLight : public BaseLight
{
    glm::vec3 Direction;
};

struct PointLight : public BaseLight
{
    glm::vec3 Position;
    glm::vec3 Attenuation; // constant, linear, exp
};

struct SpotLight : public PointLight
{
    glm::vec3 Direction;
    float Cutoff;
};

class LightingTechnique 
{
public:

    static const uint32_t numCascade = 3;
    static const uint32_t MAX_POINT_LIGHTS = 2;
    static const uint32_t MAX_SPOT_LIGHTS = 2;

    LightingTechnique();

    void initialize();
    void bind();
    void setDevice(const GraphicsDevicePtr& device);
    void setEyePositionWS(const glm::vec3& position);
    void setCascadeEndClipSpace(uint32_t i, float vClipZ);
    void setMatLightSpace(uint32_t i, const glm::mat4& mat);
    void setMatModel(const glm::mat4& mat);
    void setMatView(const glm::mat4& mat);
    void setMatProject(const glm::mat4& mat);
    void setDirectionalLight(const DirectionalLight& Light);
    void setSpotLights(uint32_t NumLights, const SpotLight* pLights);
    void setPointLights(uint32_t NumLights, const PointLight* pLights);
    void setShadowMap(uint32_t numCascade, const GraphicsTexturePtr* texture);
    void setTexWood(const GraphicsTexturePtr& texture);
    void setDebugType(int32_t type);

    GraphicsDeviceWeakPtr m_Device;
    ProgramShader m_shader;

    struct {
        GLuint Color;
        GLuint AmbientIntensity;
        GLuint DiffuseIntensity;
        GLuint Direction;
    } m_dirLightLocation;

    struct {
        GLuint Color;
        GLuint AmbientIntensity;
        GLuint DiffuseIntensity;
        GLuint Position;
        GLuint Attenuation;
    } m_pointLightsLocation[MAX_POINT_LIGHTS];

    struct {
        GLuint Color;
        GLuint AmbientIntensity;
        GLuint DiffuseIntensity;
        GLuint Position;
        GLuint Direction;
        GLuint Cutoff;
        GLuint Attenuation;
    } m_spotLightsLocation[MAX_SPOT_LIGHTS];

    GLuint m_eyePositionWSLoc;
    GLuint m_matModelLoc;
    GLuint m_matViewLoc;
    GLuint m_matProjectLoc;
    GLuint m_numPointLightsLocation;
    GLuint m_numSpotLightsLocation;
    GLuint m_cascadeEndClipSpaceLoc[numCascade];
    GLuint m_matLightLoc[numCascade];
    GLuint m_texShadowLoc[numCascade];
    GLuint m_texWoodLoc;
};