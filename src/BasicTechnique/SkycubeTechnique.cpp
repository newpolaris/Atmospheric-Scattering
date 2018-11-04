#include "skycubeTechnique.h"
#include <GL/glew.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <GLType/ProgramShader.h>
#include <tools/TCamera.h>
#include <Mesh.h>

namespace
{
    GraphicsDeviceWeakPtr m_Device; 
    GraphicsTexturePtr m_skycubeTex;
    ProgramShader m_skycubeShader;
    CubeMesh m_Cube;
}

void skycube::setDevice(const GraphicsDevicePtr &device)
{
    m_Device = device;
}

void skycube::initialize()
{
    auto device = m_Device.lock();
    assert(device);

    GraphicsTextureDesc skyDesc;
    skyDesc.setWrapS(GL_REPEAT);
    skyDesc.setWrapT(GL_REPEAT);
    skyDesc.setMinFilter(GL_LINEAR);
    skyDesc.setMagFilter(GL_LINEAR);
    skyDesc.setFilename("resources/kyoto_lod.dds");
    m_skycubeTex = device->createTexture(skyDesc);
    assert(m_skycubeTex);

	m_skycubeShader.setDevice(device);
	m_skycubeShader.initialize();
	m_skycubeShader.addShader(GL_VERTEX_SHADER, "Skycube.Vertex");
	m_skycubeShader.addShader(GL_FRAGMENT_SHADER, "Skycube.Fragment");
	m_skycubeShader.link();

    m_Cube.create();
}

void skycube::shutdown()
{
    m_Cube.destroy();
    m_Device.reset();
}

void skycube::render(GraphicsContext& gfxContext, const TCamera& camera)
{
    gfxContext.SetCubemapSeamless(true);
    gfxContext.SetFrontFace(FrontFaceType::kClockWise);
    m_skycubeShader.bind();
    m_skycubeShader.setUniform("uViewMatrix", camera.getViewMatrix());
    m_skycubeShader.setUniform("uProjMatrix", camera.getProjectionMatrix());
    m_skycubeShader.bindTexture("uEnvmapSamp", m_skycubeTex, 0);
    m_Cube.draw();
    gfxContext.SetFrontFace(FrontFaceType::kCountClockWise);
    gfxContext.SetCubemapSeamless(false);
}
