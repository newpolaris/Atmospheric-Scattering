#include "WaterTechnique.h"

#include <GL/glew.h>
#include <glfw3.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <tools/TCamera.h>

namespace
{
    const float Cycle = 0.15f;
    const float HalfCycle = Cycle * 0.5f;
    const float FlowSpeed = 0.05f;
}

void WaterTechnique::setDevice(const GraphicsDevicePtr& device)
{
    m_Device = device;
}

void WaterTechnique::create(const WaterOptions& options)
{
    auto device = m_Device.lock();
    assert(device);

    GraphicsTextureDesc flowMapDesc;
    flowMapDesc.setWrapS(GL_REPEAT);
    flowMapDesc.setWrapT(GL_REPEAT);
    flowMapDesc.setMinFilter(GL_LINEAR);
    flowMapDesc.setMagFilter(GL_LINEAR);
    flowMapDesc.setFilename("resources/WaterFlow/Textures/flowmap.png");
    m_FlowMapTex = device->createTexture(flowMapDesc);
    assert(m_FlowMapTex);

    GraphicsTextureDesc noiseMapDesc;
    noiseMapDesc.setWrapS(GL_REPEAT);
    noiseMapDesc.setWrapT(GL_REPEAT);
    noiseMapDesc.setMinFilter(GL_LINEAR);
    noiseMapDesc.setMagFilter(GL_LINEAR);
    noiseMapDesc.setFilename("resources/WaterFlow/Textures/noise.png");
    m_NoiseMapTex = device->createTexture(noiseMapDesc);
    assert(m_NoiseMapTex);

    GraphicsTextureDesc Wave0Desc;
    Wave0Desc.setWrapS(GL_REPEAT);
    Wave0Desc.setWrapT(GL_REPEAT);
    Wave0Desc.setMinFilter(GL_LINEAR);
    Wave0Desc.setMagFilter(GL_LINEAR);
    Wave0Desc.setFilename("resources/WaterFlow/Textures/wave0.dds");
    m_Wave0Tex = device->createTexture(Wave0Desc);
    assert(m_Wave0Tex);

    GraphicsTextureDesc Wave1Desc;
    Wave1Desc.setWrapS(GL_REPEAT);
    Wave1Desc.setWrapT(GL_REPEAT);
    Wave1Desc.setMinFilter(GL_LINEAR);
    Wave1Desc.setMagFilter(GL_LINEAR);
    Wave1Desc.setFilename("resources/WaterFlow/Textures/wave1.dds");
    m_Wave1Tex = device->createTexture(Wave1Desc);
    assert(m_Wave1Tex);

	m_WaterShader.setDevice(device);
	m_WaterShader.initialize();
	m_WaterShader.addShader(GL_VERTEX_SHADER, "BasicTechnique/Water.Vertex");
	m_WaterShader.addShader(GL_FRAGMENT_SHADER, "BasicTechnique/Water.Fragment");
	m_WaterShader.link();

    m_WaterPlane.create();

    m_Options.FlowMapOffset0 = 0.0f;
    m_Options.FlowMapOffset1 = HalfCycle;
}

void WaterTechnique::update(float detla)
{
    m_Options.FlowMapOffset0 += FlowSpeed * detla;
    m_Options.FlowMapOffset1 += FlowSpeed * detla;
    if (m_Options.FlowMapOffset0 >= Cycle)
        m_Options.FlowMapOffset0 = fmod(m_Options.FlowMapOffset0, Cycle);

    if (m_Options.FlowMapOffset1 >= Cycle)
        m_Options.FlowMapOffset1 = fmod(m_Options.FlowMapOffset1, Cycle);
}

void WaterTechnique::destroy()
{
    m_WaterPlane.destroy();
    m_Device.reset();
}

void WaterTechnique::render(GraphicsContext& gfxContext, const TCamera& camera)
{
    m_WaterShader.bind();
    m_WaterShader.setUniform("uCameraPositionWS", camera.getPosition());
    m_WaterShader.setUniform("uFlowMapOffset0", m_Options.FlowMapOffset0);
    m_WaterShader.setUniform("uFlowMapOffset1", m_Options.FlowMapOffset1);
    m_WaterShader.setUniform("uHalfCycle", HalfCycle);
    m_WaterShader.setUniform("uTexScale", m_Options.WaveMapScale);
    m_WaterShader.setUniform("uSunDirection", m_Options.SunDirection);
    m_WaterShader.setUniform("uSunFactor", m_Options.SunFactor);
    m_WaterShader.setUniform("uSunPower", m_Options.SunPower);
    m_WaterShader.setUniform("uMatWorld", glm::mat4(1.f));
    m_WaterShader.setUniform("uMatViewProject", camera.getViewProjMatrix());
    m_WaterShader.bindTexture("uFlowMapSamp", m_FlowMapTex, 0);
    m_WaterShader.bindTexture("uNoiseMapSamp", m_NoiseMapTex, 1);
    m_WaterShader.bindTexture("uWaveMap0Samp", m_Wave0Tex, 2);
    m_WaterShader.bindTexture("uWaveMap1Samp", m_Wave1Tex, 3);
    m_WaterPlane.draw();
}
