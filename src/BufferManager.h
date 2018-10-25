#pragma once

#include <GraphicsTypes.h>

namespace Graphics
{
    extern const uint32_t g_ShadowMapSize;
    extern const uint32_t g_NumShadowCascade;

    extern uint32_t g_NativeWidth;
    extern uint32_t g_NativeHeight;
    extern GraphicsTexturePtr g_SceneMap;
    extern GraphicsTexturePtr g_DepthMap;
    extern GraphicsFramebufferPtr g_MainFramebuffer;

    extern GraphicsTexturePtr g_ShadowMap[];
    extern GraphicsFramebufferPtr g_ShadowMapFramebuffer[];

    extern GraphicsTexturePtr g_EnvLightMap;
    extern GraphicsTexturePtr g_EnvLightAlphaMap;
    extern GraphicsFramebufferPtr g_EnvLightFramebuffer;

    extern GraphicsTexturePtr g_Gbuffer1Map;
    extern GraphicsTexturePtr g_Gbuffer2Map;
    extern GraphicsTexturePtr g_Gbuffer3Map;
    extern GraphicsTexturePtr g_Gbuffer4Map;
    extern GraphicsFramebufferPtr g_ObjectFramebuffer;

    extern GraphicsTexturePtr g_Gbuffer5Map;
    extern GraphicsTexturePtr g_Gbuffer6Map;
    extern GraphicsTexturePtr g_Gbuffer7Map;
    extern GraphicsTexturePtr g_Gbuffer8Map;
    extern GraphicsFramebufferPtr g_ObjectAlphaFramebuffer;

    void initializeRenderingBuffers(const GraphicsDevicePtr& device, uint32_t nativeWidth, uint32_t nativeHeight);
    void resizeDisplayDependentBuffers(uint32_t nativeWidth, uint32_t nativeHeight);
    void destroyRenderingBuffers();
}