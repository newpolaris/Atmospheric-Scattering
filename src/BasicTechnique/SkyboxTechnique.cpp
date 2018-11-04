#include "SkyboxTechnique.h"
#include <GL/glew.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <GLType/ProgramShader.h>
#include <tools/TCamera.h>
#include <Mesh.h>

namespace
{
    GraphicsDeviceWeakPtr m_Device; 
    GraphicsTexturePtr m_SkyboxTex;
    ProgramShader m_SkyboxShader;
    CubeMesh m_Cube;
}

void skybox::setDevice(const GraphicsDevicePtr &device)
{
    m_Device = device;
}

void skybox::initialize()
{
    auto device = m_Device.lock();
    assert(device);

    GraphicsTextureDesc skyDesc;
    skyDesc.setWrapS(GL_REPEAT);
    skyDesc.setWrapT(GL_REPEAT);
    skyDesc.setMinFilter(GL_NEAREST_MIPMAP_LINEAR);
    skyDesc.setMagFilter(GL_LINEAR);
    skyDesc.setFilename("resources/Skybox/newport_loft.hdr");
    skyDesc.setFilename("resources/Skybox/helipad.dds");
    m_SkyboxTex = device->createTexture(skyDesc);
    assert(m_SkyboxTex);

	m_SkyboxShader.setDevice(device);
	m_SkyboxShader.initialize();
	m_SkyboxShader.addShader(GL_VERTEX_SHADER, "BasicTechnique/SkyboxVS.glsl");
	m_SkyboxShader.addShader(GL_FRAGMENT_SHADER, "BasicTechnique/SkyboxFS.glsl");
	m_SkyboxShader.link();

    m_Cube.create();
}

void skybox::shutdown()
{
    m_Cube.destroy();
    m_Device.reset();
}

void skybox::render(GraphicsContext& gfxContext, const TCamera& camera)
{
    gfxContext.SetFrontFace(FrontFaceType::kClockWise);
    m_SkyboxShader.bind();
    m_SkyboxShader.setUniform("uMatViewProject", camera.getViewProjMatrix());
    m_SkyboxShader.bindTexture("uSkyboxMapSamp", m_SkyboxTex, 0);
    m_Cube.draw();
    gfxContext.SetFrontFace(FrontFaceType::kCountClockWise);
}
