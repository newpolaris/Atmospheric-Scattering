#pragma once

#include <GraphicsTypes.h>

namespace light_cube
{
    void initialize(const GraphicsDevicePtr& device);
    void shutdown();
}

class LightCube
{
public:

    LightCube();
    ~LightCube();

    void initialize(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture);
    bool update(const GraphicsDevicePtr& device);
    void destroy();

    const GraphicsTexturePtr& getEnvCube() const;
    const GraphicsTexturePtr& getIrradiance() const;
    const GraphicsTexturePtr& getPrefilter() const;

private:

    void updateCubemap(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture);
    void updateIrradiance(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture);
    void updatePrefilter(const GraphicsDevicePtr& device, const GraphicsTexturePtr& texture);

    const uint32_t m_MipmapLevels = 8;
    const uint32_t m_envMapSize = 512;
    const uint32_t m_irradianceSize = 16;
    const uint32_t m_prefilterSize = 256;

    GraphicsFramebufferPtr m_captureFBO;
    GraphicsTexturePtr m_envCubemap;
    GraphicsTexturePtr m_irradianceCubemap;
    GraphicsTexturePtr m_prefilterCubemap;
};
