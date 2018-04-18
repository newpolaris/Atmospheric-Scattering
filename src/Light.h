#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <GraphicsTypes.h>

typedef std::shared_ptr<class ProgramShader> ShaderPtr;

struct RenderingData
{
    bool bGroudTruth;
    glm::vec3 Position;
    glm::mat4 View;
    glm::mat4 Projection;
    std::vector<glm::vec4> Samples;
};

namespace light
{
    void initialize(const GraphicsDevicePtr& device);
    void shutdown();
}

class Light
{
public:

    static ShaderPtr BindProgram(const RenderingData& data, bool bDepth);
    static ShaderPtr BindLightProgram(const RenderingData& data, bool bDepth);

    Light() noexcept;

    ShaderPtr submit(ShaderPtr& shader, bool bDepth);
    ShaderPtr submitPerLightUniforms(const RenderingData& data, ShaderPtr& shader);

    glm::mat4 getWorld() const;

    const glm::vec3& getPosition() noexcept;
    void setPosition(const glm::vec3& position) noexcept;
    const glm::vec3& getRotation() noexcept;
    void setRotation(const glm::vec3& rotation) noexcept;
    const float getIntensity() noexcept;
    void setIntensity(float intensity) noexcept;
    void setTexturedLight(bool bTextured) noexcept;
    void setLightSource(const GraphicsTexturePtr& texture) noexcept;
    void setLightFilterd(const GraphicsTexturePtr& texture) noexcept;

    glm::vec3 m_Position; // where are we
    glm::vec3 m_Rotation;
    glm::vec4 m_Specular;

    float m_Width;
    float m_Height;
    float m_Intensity;
    bool m_bTwoSided;
    bool m_bTexturedLight;

    GraphicsTexturePtr m_LightSourceTex;
    GraphicsTexturePtr m_LightFilteredTex;
};
