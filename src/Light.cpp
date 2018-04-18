#include <Light.h>
#include <Mesh.h>
#include <GLType/ProgramShader.h>
#include <GL/glew.h>
#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLCoreGraphicsData.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace light
{
    PlaneMesh m_LightMesh(2.0, 1.0);
    GraphicsTexturePtr m_Ltc1Tex;
    GraphicsTexturePtr m_Ltc2Tex;
    GraphicsTexturePtr m_WhiteTex;
    ShaderPtr m_ShaderLight;
    ShaderPtr m_ShaderGroudTruth;
    ShaderPtr m_ShaderDepthLight;
    ShaderPtr m_ShaderLTC;
    ShaderPtr m_ShaderDepthLTC;

    void initialize(const GraphicsDevicePtr& device)
    {
        m_ShaderLight = std::make_shared<ProgramShader>();
        m_ShaderLight->setDevice(device);
        m_ShaderLight->initialize();
        m_ShaderLight->addShader(GL_VERTEX_SHADER, "TexturedLight.Vertex");
        m_ShaderLight->addShader(GL_FRAGMENT_SHADER, "TexturedLight.Fragment");
        m_ShaderLight->link();

        m_ShaderDepthLight = std::make_shared<ProgramShader>();
        m_ShaderDepthLight->setDevice(device);
        m_ShaderDepthLight->initialize();
        m_ShaderDepthLight->addShader(GL_VERTEX_SHADER, "DepthLight.Vertex");
	#if __APPLE__
        m_ShaderDepthLight->addShader(GL_FRAGMENT_SHADER, "DepthLight.Fragment");
	#endif
        m_ShaderDepthLight->link();

        m_ShaderLTC = std::make_shared<ProgramShader>();
        m_ShaderLTC->setDevice(device);
        m_ShaderLTC->initialize();
        m_ShaderLTC->addShader(GL_VERTEX_SHADER, "Ltc.Vertex");
        m_ShaderLTC->addShader(GL_FRAGMENT_SHADER, "Ltc.Fragment");
        m_ShaderLTC->link();

        m_ShaderDepthLTC = std::make_shared<ProgramShader>();
        m_ShaderDepthLTC->setDevice(device);
        m_ShaderDepthLTC->initialize();
        m_ShaderDepthLTC->addShader(GL_VERTEX_SHADER, "DepthLtc.Vertex");
	#if __APPLE__
        m_ShaderDepthLTC->addShader(GL_FRAGMENT_SHADER, "DepthLtc.Fragment");
	#endif
        m_ShaderDepthLTC->link();

        m_ShaderGroudTruth  = std::make_shared<ProgramShader>();
        m_ShaderGroudTruth->setDevice(device);
        m_ShaderGroudTruth->initialize();
        m_ShaderGroudTruth->addShader(GL_VERTEX_SHADER, "GroundTruth.Vertex");
        m_ShaderGroudTruth->addShader(GL_FRAGMENT_SHADER, "GroundTruth.Fragment");
        m_ShaderGroudTruth->link();

        m_LightMesh.create();

        GraphicsTextureDesc source;
        source.setFilename("resources/white.png");
        m_WhiteTex = device->createTexture(source);

        GraphicsTextureDesc ltcMatDesc;
        ltcMatDesc.setFilename("resources/ltc_1.dds");
        ltcMatDesc.setWrapS(GL_CLAMP_TO_EDGE);
        ltcMatDesc.setWrapT(GL_CLAMP_TO_EDGE);
        ltcMatDesc.setMinFilter(GL_NEAREST);
        ltcMatDesc.setMagFilter(GL_LINEAR);
        m_Ltc1Tex = device->createTexture(ltcMatDesc);

        GraphicsTextureDesc ltcMagDesc;
        ltcMagDesc.setFilename("resources/ltc_2.dds");
        ltcMagDesc.setWrapS(GL_CLAMP_TO_EDGE);
        ltcMagDesc.setWrapT(GL_CLAMP_TO_EDGE);
        ltcMagDesc.setMinFilter(GL_NEAREST);
        ltcMagDesc.setMagFilter(GL_LINEAR);
        m_Ltc2Tex = device->createTexture(ltcMagDesc);
    }

    void shutdown()
    {
        m_LightMesh.destroy();
    }
}

using namespace light;

Light::Light() noexcept
    : m_Position(0.f)
    , m_Rotation(0.f)
    , m_Width(8.f)
    , m_Height(8.f)
    , m_Intensity(4.f)
    , m_bTwoSided(false)
    , m_bTexturedLight(false)
{
}

ShaderPtr Light::BindProgram(const RenderingData& data, bool bDepth)
{
    auto program = data.bGroudTruth ? m_ShaderGroudTruth : m_ShaderLTC;
    program = bDepth ? m_ShaderDepthLTC : program;

    program->bind();
    program->setUniform("uView", data.View);
    program->setUniform("uProjection", data.Projection);
    if (!bDepth)
    {
        program->setUniform("uViewPositionW", data.Position);
        if (data.bGroudTruth)
            program->setUniform("uSamples", data.Samples.data(), data.Samples.size());
        else
        {
            program->bindTexture("uLtc1", m_Ltc1Tex, 0);
            program->bindTexture("uLtc2", m_Ltc2Tex, 1);
        }
    }
    return program;
}

ShaderPtr Light::BindLightProgram(const RenderingData& data, bool bDepth)
{
    auto shader = bDepth ? m_ShaderDepthLight : m_ShaderLight;
    shader->bind();
    shader->setUniform("uViewProj", data.Projection*data.View);
    return shader;
}

ShaderPtr Light::submit(ShaderPtr& shader, bool bDepth)
{
    glm::mat4 world = getWorld();
    shader->setUniform("uWorld", world);
    if (!bDepth)
    {
        auto& sourceTex = m_bTexturedLight ? m_LightSourceTex : m_WhiteTex;
        shader->setUniform("ubTexturedLight", m_bTexturedLight);
        shader->setUniform("uIntensity", m_Intensity);
        shader->bindTexture("uTexColor", sourceTex, 0);
    }
    m_LightMesh.draw();

    return shader;
}

ShaderPtr Light::submitPerLightUniforms(const RenderingData& data, ShaderPtr& shader)
{
    // local
    glm::mat4 model = getWorld();
    // area light rect poinsts in world space
    glm::vec4 points[] = 
    {
        { -1.f, 0.f, -1.f, 1.f },
        { +1.f, 0.f, -1.f, 1.f },
        { +1.f, 0.f, +1.f, 1.f },
        { -1.f, 0.f, +1.f, 1.f },
    };

    points[0] = model * points[0];
    points[1] = model * points[1];
    points[2] = model * points[2];
    points[3] = model * points[3];

    shader->setUniform("ubTwoSided", m_bTwoSided);
    if (!data.bGroudTruth)
        shader->setUniform("ubTexturedLight", m_bTexturedLight);
    shader->setUniform("uIntensity", m_Intensity);
    shader->setUniform("uQuadPoints", points, 4);

    if (m_bTexturedLight && !data.bGroudTruth)
        shader->bindTexture("uFilteredMap", m_LightFilteredTex, 2);
    if (data.bGroudTruth)
    {
        auto& texture = m_bTexturedLight ? m_LightSourceTex : m_WhiteTex;
        shader->bindTexture("uTexColor", texture, 0);
    }
    return shader;
}

glm::mat4 Light::getWorld() const
{
    glm::mat4 identity = glm::mat4(1.f);
    glm::mat4 translate = glm::translate(identity, glm::vec3(m_Position));
    glm::mat4 rotateZ = glm::rotate(identity, glm::radians(m_Rotation.z), glm::vec3(0, 0, 1));
    glm::mat4 rotateY = glm::rotate(identity, glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
    glm::mat4 rotateX = glm::rotate(identity, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
    glm::mat4 scale = glm::scale(identity, glm::vec3(m_Width, 1, m_Height));
    return translate*rotateX*rotateY*rotateZ*scale;
}

const glm::vec3& Light::getPosition() noexcept
{
    return m_Position;
}

void Light::setPosition(const glm::vec3& position) noexcept
{
    m_Position = position;
}

const glm::vec3& Light::getRotation() noexcept
{
    return m_Rotation;
}

void Light::setRotation(const glm::vec3& rotation) noexcept
{
    m_Rotation = rotation;
}

const float Light::getIntensity() noexcept
{
    return m_Intensity;
}

void Light::setIntensity(float intensity) noexcept
{
    m_Intensity = intensity;
}

void Light::setTexturedLight(bool bTextured) noexcept
{
    m_bTexturedLight = bTextured;
}

void Light::setLightSource(const GraphicsTexturePtr& texture) noexcept
{
    m_LightSourceTex = texture;
}

void Light::setLightFilterd(const GraphicsTexturePtr& texture) noexcept
{
    m_LightFilteredTex = texture;
}
