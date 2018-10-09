#include "LightCube.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

#include <Mesh.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <GLType/GraphicsFramebuffer.h>
#include <GLType/ProgramShader.h>
#include <GLType/OGLCoreTexture.h>
#include <tools/gltools.hpp>
#include <tools/Profile.h>

namespace light_cube
{
    const uint32_t s_brdfSize = 512;

    GraphicsTexturePtr s_brdfTexture;

    ProgramShader s_equirectangularToCubemapShader;
    ProgramShader s_programIrradiance;
    ProgramShader s_programPrefilter;
    ProgramShader s_programBrdfLut;

    FullscreenTriangleMesh s_triangle;
    CubeMesh s_cube;
}

using namespace light_cube;

void light_cube::initialize(const GraphicsDevicePtr& device)
{
    s_equirectangularToCubemapShader.setDevice(device);
    s_equirectangularToCubemapShader.initialize();
    s_equirectangularToCubemapShader.addShader(GL_VERTEX_SHADER, "Cubemap.Vertex");
    s_equirectangularToCubemapShader.addShader(GL_FRAGMENT_SHADER, "EquirectangularToCubemap.Fragment");
    s_equirectangularToCubemapShader.link();

    s_programIrradiance.setDevice(device);
    s_programIrradiance.initialize();
    s_programIrradiance.addShader(GL_COMPUTE_SHADER, "IBL/Irradiance.Compute");
    s_programIrradiance.link();

    s_programPrefilter.setDevice(device);
    s_programPrefilter.initialize();
    s_programPrefilter.addShader(GL_COMPUTE_SHADER, "IBL/Radiance.Compute");
    s_programPrefilter.link();

    s_programBrdfLut.setDevice(device);
    s_programBrdfLut.initialize();
    s_programBrdfLut.addShader(GL_COMPUTE_SHADER, "IBL/BrdfLut.Compute");
    s_programBrdfLut.link();

    s_triangle.create();
    s_cube.create();

    // create brdf lut texture
    // Generate a 2D LUT from the BRDF quation used.
    GraphicsTextureDesc brdf;
    brdf.setWidth(s_brdfSize);
    brdf.setHeight(s_brdfSize);
    brdf.setWrapS(GL_CLAMP_TO_EDGE);
    brdf.setWrapT(GL_CLAMP_TO_EDGE);
    brdf.setMinFilter(GL_LINEAR);
    brdf.setMagFilter(GL_LINEAR);
    brdf.setFormat(gli::FORMAT_RG16_SFLOAT_PACK16);
    auto tex = device->createTexture(brdf);

    // solve diffuse integral by convolution to create an irradiance cbuemap
    const int localSize = 16;
    s_programBrdfLut.bind();
    s_programBrdfLut.bindImage("uLUT", tex, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY);
    s_programBrdfLut.Dispatch2D(s_brdfSize, s_brdfSize, localSize, localSize);
    s_brdfTexture = tex;
}

void light_cube::shutdown()
{
    s_cube.destroy();
    s_triangle.destroy();
}

void LightCube::updateCubemap(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture)
{
    // set up projection and view matrices for capturing data onto the 6 cubemap face directions
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    assert(m_captureFBO != nullptr);
    device->setFramebuffer(m_captureFBO);

    auto attachment = m_captureFBO->getGraphicsFramebufferDesc().getComponents().front();
    auto desc = attachment.getTexture()->getGraphicsTextureDesc();

    // don't forget to configure the viewport to the capture dimensions.
    glViewport(0, 0, desc.getWidth(), desc.getHeight());

    s_equirectangularToCubemapShader.bind();
    // convert HDR equirectangular environment map to cubemap equivalent
    s_equirectangularToCubemapShader.bindTexture("equirectangularMap", texture, 0);
    s_equirectangularToCubemapShader.setUniform("projection", captureProjection);

    glDisable(GL_CULL_FACE);
    for (unsigned int i = 0; i < 6; ++i)
    {
        s_equirectangularToCubemapShader.setUniform("view", captureViews[i]);
        device->bindRenderTexture(m_envCubemap, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        s_cube.draw();
    }
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    device->generateMipmap(m_envCubemap);
}

void LightCube::updateIrradiance(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture)
{
    // solve diffuse integral by convolution to create an irradiance cbuemap
    const int localSize = 16;
    s_programIrradiance.bind();
    s_programIrradiance.bindTexture("uEnvMap", texture, 0);

    // Set layered true to use whole cube face
    s_programIrradiance.bindImage("uCube", m_irradianceCubemap, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY);
    s_programIrradiance.Dispatch3D(m_irradianceSize, m_irradianceSize, 6, localSize, localSize, 1);
}

void LightCube::updatePrefilter(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture)
{
    // run a quasi monte-carlo simulation on the environment lighting to create a prefilter cubemap

    // prefilter has half size
    assert(m_envMapSize/2 == m_prefilterSize);

    auto desc = texture->getGraphicsTextureDesc();
    auto targetDesc = m_prefilterCubemap->getGraphicsTextureDesc();
    assert(desc.getWidth()/2 == targetDesc.getWidth());

    assert(desc.getLevels() > 0);
    assert(targetDesc.getLevels() > 1);

    auto src = texture->downcast_pointer<OGLCoreTexture>();
    auto det = m_prefilterCubemap->downcast_pointer<OGLCoreTexture>();
    const int cubemap_count = 6;
    // copy 2nd level to 1st level
    glCopyImageSubData(
        src->getTextureID(), GL_TEXTURE_CUBE_MAP, 1, 0, 0, 0,
        det->getTextureID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        m_prefilterSize, m_prefilterSize, cubemap_count);

    const int localSize = 16;
    // Skip mipLevel 0
    auto size = m_prefilterSize / 2;
    auto mipLevel = 1;
    auto maxLevel = int(glm::floor(glm::log2(float(size))));

    s_programPrefilter.bind();
    s_programPrefilter.bindTexture("uEnvMap", texture, 0);
    for (auto tsize = size; tsize > 0; tsize /= 2)
    {
        s_programPrefilter.setUniform("uRoughness", float(mipLevel) / maxLevel);
        // Set layered true to use whole cube face
        s_programPrefilter.bindImage("uCube", m_prefilterCubemap, 0, mipLevel, GL_TRUE, 0, GL_WRITE_ONLY);
        s_programPrefilter.Dispatch3D(tsize, tsize, 6, localSize, localSize, 1);
        mipLevel++;
    }
}

void LightCube::initialize(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture)
{
    // create an irradiance cubemap
    GraphicsTextureDesc irrDesc;
    irrDesc.setWidth(m_irradianceSize);
    irrDesc.setHeight(m_irradianceSize);
    irrDesc.setTarget(gli::TARGET_CUBE);
    irrDesc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);
    irrDesc.setMinFilter(GL_LINEAR);
    m_irradianceCubemap = device->createTexture(irrDesc);

    // create a prefilter cubemap and allocate mips
    GraphicsTextureDesc prefilterDesc;
    prefilterDesc.setWidth(m_prefilterSize);
    prefilterDesc.setHeight(m_prefilterSize);
    prefilterDesc.setLevels(m_MipmapLevels);
    prefilterDesc.setTarget(gli::TARGET_CUBE);
    prefilterDesc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);
    prefilterDesc.setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
    m_prefilterCubemap = device->createTexture(prefilterDesc);

    // For Env. capture
    GraphicsTextureDesc envDesc;
    envDesc.setWidth(m_envMapSize);
    envDesc.setHeight(m_envMapSize);
    envDesc.setLevels(m_MipmapLevels);
    envDesc.setTarget(gli::TARGET_CUBE);
    envDesc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);
    envDesc.setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
    envDesc.setMagFilter(GL_LINEAR);
    m_envCubemap = device->createTexture(envDesc);

    GraphicsTextureDesc depthDesc;
    depthDesc.setWidth(m_envMapSize);
    depthDesc.setHeight(m_envMapSize);
    depthDesc.setFormat(gli::FORMAT_D24_UNORM_S8_UINT_PACK32);
    auto depth = device->createTexture(depthDesc);

    GraphicsFramebufferDesc desc;
    desc.addComponent(GraphicsAttachmentBinding(m_envCubemap, GL_COLOR_ATTACHMENT0));
    desc.addComponent(GraphicsAttachmentBinding(depth, GL_DEPTH_ATTACHMENT));
    m_captureFBO = device->createFramebuffer(desc);

    updateCubemap(device, texture);
}

bool LightCube::update(const GraphicsDevicePtr& device)
{
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    updateIrradiance(device, m_envCubemap);
    updatePrefilter(device, m_envCubemap);
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    return true;
}

void LightCube::destroy()
{
}

const GraphicsTexturePtr& LightCube::getEnvCube() const
{
    return m_envCubemap;
}

const GraphicsTexturePtr& LightCube::getIrradiance() const
{
    return m_irradianceCubemap;
}

const GraphicsTexturePtr& LightCube::getPrefilter() const
{
    return m_prefilterCubemap;
}

const GraphicsTexturePtr& LightCube::getBrdfLUT() const
{
    return s_brdfTexture;
}

LightCube::LightCube()
{
}

LightCube::~LightCube()
{
}

