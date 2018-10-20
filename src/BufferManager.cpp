#include "BufferManager.h"
#include <GL/glew.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <GLType/GraphicsFramebuffer.h>

namespace Graphics
{
    uint32_t g_NativeWidth = 0;
    uint32_t g_NativeHeight = 0;

    GraphicsDeviceWeakPtr g_DeviceWeakPtr;

    GraphicsTexturePtr g_SceneMap;
    GraphicsTexturePtr g_DepthMap;
    GraphicsTexturePtr g_ScreenDepthMap;

    GraphicsTexturePtr g_EnvLightMap;
    GraphicsTexturePtr g_EnvLightAlphaMap;

    GraphicsTexturePtr g_Gbuffer1Map;
    GraphicsTexturePtr g_Gbuffer2Map;
    GraphicsTexturePtr g_Gbuffer3Map;
    GraphicsTexturePtr g_Gbuffer4Map;
    GraphicsTexturePtr g_Gbuffer5Map;
    GraphicsTexturePtr g_Gbuffer6Map;
    GraphicsTexturePtr g_Gbuffer7Map;
    GraphicsTexturePtr g_Gbuffer8Map;

    GraphicsFramebufferPtr g_MainFramebuffer;
    GraphicsFramebufferPtr g_EnvLightFramebuffer;
    GraphicsFramebufferPtr g_ObjectFramebuffer;
    GraphicsFramebufferPtr g_ObjectAlphaFramebuffer;
}

void Graphics::initializeRenderingBuffers(const GraphicsDevicePtr& device, uint32_t nativeWidth, uint32_t nativeHeight)
{
    g_DeviceWeakPtr = device;
    assert(device);

    g_NativeWidth = nativeWidth;
    g_NativeHeight = nativeHeight;

    GraphicsTextureDesc sceneDesc;
    sceneDesc.setWrapS(GL_CLAMP_TO_EDGE);
    sceneDesc.setWrapT(GL_CLAMP_TO_EDGE);
    sceneDesc.setMinFilter(GL_NEAREST);
    sceneDesc.setMagFilter(GL_NEAREST);
    sceneDesc.setWidth(nativeWidth);
    sceneDesc.setHeight(nativeHeight);
    sceneDesc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);
    g_SceneMap = device->createTexture(sceneDesc);

    GraphicsTextureDesc depthDesc;
    depthDesc.setWidth(nativeWidth);
    depthDesc.setHeight(nativeHeight);
    depthDesc.setFormat(gli::FORMAT_D24_UNORM_S8_UINT_PACK32);

    g_DepthMap = device->createTexture(depthDesc);
    g_ScreenDepthMap = device->createTexture(depthDesc);

    GraphicsFramebufferDesc mainFrameDesc;
    mainFrameDesc.addComponent(GraphicsAttachmentBinding(g_SceneMap, GL_COLOR_ATTACHMENT0));
    mainFrameDesc.addComponent(GraphicsAttachmentBinding(g_DepthMap, GL_DEPTH_STENCIL_ATTACHMENT));
    g_MainFramebuffer = device->createFramebuffer(mainFrameDesc);

    GraphicsTextureDesc envLightDesc;
    envLightDesc.setWrapS(GL_CLAMP_TO_EDGE);
    envLightDesc.setWrapT(GL_CLAMP_TO_EDGE);
    envLightDesc.setMinFilter(GL_NEAREST);
    envLightDesc.setMagFilter(GL_NEAREST);
    envLightDesc.setWidth(nativeWidth);
    envLightDesc.setHeight(nativeHeight);
    envLightDesc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);
    g_EnvLightMap = device->createTexture(envLightDesc);
    g_EnvLightAlphaMap = device->createTexture(envLightDesc);

    GraphicsFramebufferDesc diffuseFrameDesc;
    diffuseFrameDesc.addComponent(GraphicsAttachmentBinding(g_EnvLightMap, GL_COLOR_ATTACHMENT0));
    diffuseFrameDesc.addComponent(GraphicsAttachmentBinding(g_EnvLightAlphaMap, GL_COLOR_ATTACHMENT1));
    g_EnvLightFramebuffer = device->createFramebuffer(diffuseFrameDesc);

    GraphicsTextureDesc gbuffer1Desc;
    gbuffer1Desc.setWrapS(GL_CLAMP_TO_EDGE);
    gbuffer1Desc.setWrapT(GL_CLAMP_TO_EDGE);
    gbuffer1Desc.setMinFilter(GL_NEAREST);
    gbuffer1Desc.setMagFilter(GL_NEAREST);
    gbuffer1Desc.setWidth(nativeWidth);
    gbuffer1Desc.setHeight(nativeHeight);
    gbuffer1Desc.setFormat(gli::FORMAT_RGBA8_UNORM_PACK32);

    GraphicsTextureDesc gbuffer2Desc = gbuffer1Desc;
    gbuffer2Desc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);

    g_Gbuffer1Map = device->createTexture(gbuffer1Desc);
    g_Gbuffer2Map = device->createTexture(gbuffer1Desc);
    g_Gbuffer3Map = device->createTexture(gbuffer1Desc);
    g_Gbuffer4Map = device->createTexture(gbuffer2Desc);

    GraphicsFramebufferDesc gbufferFrame1Desc;
    gbufferFrame1Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer1Map, GL_COLOR_ATTACHMENT0));
    gbufferFrame1Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer2Map, GL_COLOR_ATTACHMENT1));
    gbufferFrame1Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer3Map, GL_COLOR_ATTACHMENT2));
    gbufferFrame1Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer4Map, GL_COLOR_ATTACHMENT3));
    gbufferFrame1Desc.addComponent(GraphicsAttachmentBinding(g_ScreenDepthMap, GL_DEPTH_ATTACHMENT));
    g_ObjectFramebuffer = device->createFramebuffer(gbufferFrame1Desc);

    g_Gbuffer5Map = device->createTexture(gbuffer1Desc);
    g_Gbuffer6Map = device->createTexture(gbuffer1Desc);
    g_Gbuffer7Map = device->createTexture(gbuffer1Desc);
    g_Gbuffer8Map = device->createTexture(gbuffer2Desc);

    GraphicsFramebufferDesc gbufferFrame2Desc;
    gbufferFrame2Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer5Map, GL_COLOR_ATTACHMENT0));
    gbufferFrame2Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer6Map, GL_COLOR_ATTACHMENT1));
    gbufferFrame2Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer7Map, GL_COLOR_ATTACHMENT2));
    gbufferFrame2Desc.addComponent(GraphicsAttachmentBinding(g_Gbuffer8Map, GL_COLOR_ATTACHMENT3));
    gbufferFrame2Desc.addComponent(GraphicsAttachmentBinding(g_ScreenDepthMap, GL_DEPTH_ATTACHMENT));
    g_ObjectAlphaFramebuffer = device->createFramebuffer(gbufferFrame2Desc);
}

void Graphics::resizeDisplayDependentBuffers(uint32_t nativeWidth, uint32_t nativeHeight)
{
    auto device = g_DeviceWeakPtr.lock();
    assert(device);
}

void Graphics::destroyRenderingBuffers()
{
    g_EnvLightFramebuffer.reset();
    g_EnvLightMap.reset();
    g_EnvLightAlphaMap.reset();

    g_Gbuffer1Map.reset();
    g_Gbuffer2Map.reset();
    g_Gbuffer3Map.reset();
    g_Gbuffer4Map.reset();

    g_Gbuffer5Map.reset();
    g_Gbuffer6Map.reset();
    g_Gbuffer7Map.reset();
    g_Gbuffer8Map.reset();
}
