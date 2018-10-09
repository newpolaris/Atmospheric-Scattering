#include "Skybox/Skybox.h"

#include <GL/glew.h>
#include <GLType/ProgramShader.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <Mesh.h>

#define IBLDIFF_MAP_FILE "resources/Skybox/helipaddiff_hdr.dds"
#define IBLSPEC_MAP_FILE "resources/Skybox/helipadspec_hdr.dds"
#define IBL_MIPMAP_LEVEL 7

namespace skybox
{
    CubeMesh s_CubeMesh;
    FullscreenTriangleMesh s_ScreenTraingle;
    GraphicsTexturePtr s_BrdfSamp;
    GraphicsTexturePtr s_DiffuseSamp;
    GraphicsTexturePtr s_SpecularSamp;
    
    ProgramShader s_EnvLighting;
}

void skybox::initialize(const GraphicsDevicePtr& device)
{
    s_CubeMesh.create();
    s_ScreenTraingle.create();

    GraphicsTextureDesc brdf;
    brdf.setWrapS(GL_CLAMP);
    brdf.setWrapT(GL_CLAMP);
    brdf.setMinFilter(GL_LINEAR);
    brdf.setMagFilter(GL_LINEAR);
    brdf.setFilename("resources/Skybox/BRDF.hdr");
    s_BrdfSamp = device->createTexture(brdf);

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
}

void skybox::render(const TCamera& camera)
{
    glFrontFace(GL_CW);
    s_EnvLighting.bind();
    s_EnvLighting.setUniform("uModelToProj", camera.getViewProjMatrix());
    s_EnvLighting.bindTexture("uTextureSamp", s_SpecularSamp, 0);
    s_CubeMesh.draw();
    glFrontFace(GL_CCW);
}
