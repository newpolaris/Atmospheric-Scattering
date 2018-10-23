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

    static const uint32_t MAX_POINT_LIGHTS = 2;
    static const uint32_t MAX_SPOT_LIGHTS = 2;

    LightingTechnique();

    void initialize();
    void bind();
    void setDevice(const GraphicsDevicePtr& device);
    void setEyePositionWS(const glm::vec3& position);
    void setMatLight(const glm::mat4& mat);
    void setMatWorld(const glm::mat4& mat);
    void setMatWorldViewProject(const glm::mat4& mat);
    void setDirectionalLight(const DirectionalLight& Light);
    void setSpotLights(uint32_t NumLights, const SpotLight* pLights);
    void setPointLights(uint32_t NumLights, const PointLight* pLights);
    void setShadowMap(const GraphicsTexturePtr& texture);

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
    GLuint m_matLightLoc;
    GLuint m_matWorldLoc;
    GLuint m_matWorldViewProjectLoc;
    GLuint m_numPointLightsLocation;
    GLuint m_numSpotLightsLocation;
    GLuint m_texShadowLoc;
};