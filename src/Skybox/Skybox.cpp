#include "Skybox/Skybox.h"

#include <GL/glew.h>
#include <GLType/ProgramShader.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <GLType/GraphicsFramebuffer.h>
#include <Mesh.h>
#include <BufferManager.h>

#define SKYBOX_MAP_FILE "resources/skybox/helipad.dds"
#define IBLDIFF_MAP_FILE "resources/Skybox/helipaddiff_hdr.dds"
#define IBLSPEC_MAP_FILE "resources/Skybox/helipadspec_hdr.dds"
#define IBL_MIPMAP_LEVEL 7

namespace skybox
{
    CubeMesh s_CubeMesh;
    SphereMesh s_SphereMesh(64, 1e3f);
    GraphicsTexturePtr s_BrdfSamp;
    GraphicsTexturePtr s_SkyboxSamp;
    GraphicsTexturePtr s_DiffuseSamp;
    GraphicsTexturePtr s_SpecularSamp;
    ProgramShader s_EnvLighting;
    GraphicsDevicePtr s_Device;
}

void skybox::initialize(const GraphicsDevicePtr& device)
{
    s_Device = device;

    s_CubeMesh.create();
    s_SphereMesh.create();

    GraphicsTextureDesc brdf;
    brdf.setWrapS(GL_CLAMP);
    brdf.setWrapT(GL_CLAMP);
    brdf.setMinFilter(GL_LINEAR);
    brdf.setMagFilter(GL_LINEAR);
    brdf.setFilename("resources/Skybox/BRDF.hdr");
    s_BrdfSamp = device->createTexture(brdf);

    GraphicsTextureDesc skybox;
    skybox.setWrapS(GL_REPEAT);
    skybox.setWrapT(GL_REPEAT);
    skybox.setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
    skybox.setMagFilter(GL_LINEAR);
    skybox.setFilename(SKYBOX_MAP_FILE);
    s_SkyboxSamp = device->createTexture(skybox);

    GraphicsTextureDesc diffuse;
    diffuse.setWrapS(GL_CLAMP);
    diffuse.setWrapT(GL_CLAMP);
    diffuse.setMinFilter(GL_LINEAR);
    diffuse.setMagFilter(GL_LINEAR);
    diffuse.setFilename(IBLDIFF_MAP_FILE);
    s_DiffuseSamp = device->createTexture(diffuse);

    GraphicsTextureDesc specular;
    specular.setWrapS(GL_CLAMP);
    specular.setWrapT(GL_CLAMP);
    specular.setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
    specular.setMagFilter(GL_LINEAR);
    specular.setFilename(IBLSPEC_MAP_FILE);
    specular.setLevels(IBL_MIPMAP_LEVEL);
    s_SpecularSamp = device->createTexture(specular);

	s_EnvLighting.setDevice(device);
	s_EnvLighting.initialize();
	s_EnvLighting.addShader(GL_VERTEX_SHADER, "Helipad GoldenHour/EnvLightingVS.glsl");
	s_EnvLighting.addShader(GL_FRAGMENT_SHADER, "Helipad GoldenHour/EnvLightingPS.glsl");
	s_EnvLighting.link();
}

void skybox::shutdown()
{
    s_CubeMesh.destroy();
    s_SphereMesh.destroy();
    s_Device = nullptr;
}

void skybox::light(const TCamera& camera)
{
    #define MIDPOINT_8_BIT (127.0f / 255.0f)

    const GraphicsTextureDesc& desc = Graphics::g_EnvLightMap->getGraphicsTextureDesc(); 
    s_Device->setFramebuffer(Graphics::g_EnvLightFramebuffer);
    glViewport(0, 0, desc.getWidth(), desc.getHeight());
    // glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClearColor(0.0f, MIDPOINT_8_BIT, 0.0f, MIDPOINT_8_BIT);
    // glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glClearColor(0.0f, MIDPOINT_8_BIT, 0.0f, MIDPOINT_8_BIT);
    glClear(GL_COLOR_BUFFER_BIT);

    auto matViewInverse = glm::inverse(camera.getViewMatrix());
    glFrontFace(GL_CW);
    s_EnvLighting.bind();
    s_EnvLighting.setUniform("uMatViewInverse", matViewInverse);
    s_EnvLighting.setUniform("uModelToProj", camera.getViewProjMatrix());
    s_EnvLighting.bindTexture("uBRDFSamp", s_BrdfSamp, 0);
    s_EnvLighting.bindTexture("uDiffuseSamp", s_DiffuseSamp, 1);
    s_EnvLighting.bindTexture("uSpecularSamp", s_SpecularSamp, 2);
    s_EnvLighting.bindTexture("uGbuffer1", Graphics::g_Gbuffer1Map, 3);
    s_EnvLighting.bindTexture("uGbuffer2", Graphics::g_Gbuffer2Map, 4);
    s_EnvLighting.bindTexture("uGbuffer3", Graphics::g_Gbuffer3Map, 5);
    s_EnvLighting.bindTexture("uGbuffer4", Graphics::g_Gbuffer4Map, 6);
    s_EnvLighting.bindTexture("uGbuffer5", Graphics::g_Gbuffer5Map, 7);
    s_EnvLighting.bindTexture("uGbuffer6", Graphics::g_Gbuffer6Map, 8);
    s_EnvLighting.bindTexture("uGbuffer7", Graphics::g_Gbuffer7Map, 9);
    s_EnvLighting.bindTexture("uGbuffer8", Graphics::g_Gbuffer8Map, 10);
    s_SphereMesh.draw();
    glFrontFace(GL_CCW);
}

void skybox::render(const TCamera& camera)
{
    glFrontFace(GL_CW);
    s_EnvLighting.bind();
    s_EnvLighting.setUniform("uModelToProj", camera.getViewProjMatrix());
    s_EnvLighting.bindTexture("uTextureSamp", s_SkyboxSamp, 0);
    s_CubeMesh.draw();
    glFrontFace(GL_CCW);
}