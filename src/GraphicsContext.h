#pragma once

#include <GraphicsTypes.h>

enum FrontFaceType { kCountClockWise = 0, kClockWise };
enum CullFaceType { kCullFront, kCullBack, kCullAll };
enum ClearBitType
{
    kColorBufferBit = 1,
    kDepthBufferBit = 2,
    kStencilBufferBit = 4,
    kAccumBufferBit = 8
};

class GraphicsContext
{
public:

    GraphicsContext(GraphicsDeviceType type);

    void Clear(uint8_t flags);
    void ClearColor(glm::vec4 color);
    void ClearDepth(float depth);

    void SetViewport(int x, int y, size_t width, size_t height);
    void SetFrontFace(FrontFaceType flag);
    void SetCullFace(CullFaceType flag);
    void SetDepthClamp(bool bFlag);
    void SetDepthTest(bool bFlag);
    void SetCubemapSeamless(bool bFlag);
    void SetFramebuffer(const GraphicsFramebufferPtr& framebuffer) noexcept;

    GraphicsDeviceType m_DeviceType;
};

