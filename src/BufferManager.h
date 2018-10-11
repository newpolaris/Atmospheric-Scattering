#pragma once

#include <GraphicsTypes.h>

namespace Graphics
{
    extern GraphicsTexturePtr g_EnvLightMap;
    extern GraphicsTexturePtr g_EnvLightAlphaMap;
    extern GraphicsFramebufferPtr g_EnvLightFramebuffer;

    extern GraphicsTexturePtr g_Gbuffer1Map;
    extern GraphicsTexturePtr g_Gbuffer2Map;
    extern GraphicsTexturePtr g_Gbuffer3Map;
    extern GraphicsTexturePtr g_Gbuffer4Map;

    extern GraphicsTexturePtr g_Gbuffer5Map;
    extern GraphicsTexturePtr g_Gbuffer6Map;
    extern GraphicsTexturePtr g_Gbuffer7Map;
    extern GraphicsTexturePtr g_Gbuffer8Map;

    void initializeRenderingBuffers(const GraphicsDevicePtr& device, uint32_t nativeWidth, uint32_t nativeHeight);
    void resizeDisplayDependentBuffers(uint32_t nativeWidth, uint32_t nativeHeight);
    void destroyRenderingBuffers();
}