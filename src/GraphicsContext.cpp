#include "GraphicsContext.h"

#include <GL/glew.h>
#include <GLType/OGLFramebuffer.h>
#include <GLType/OGLCoreFramebuffer.h>

GraphicsContext::GraphicsContext(GraphicsDeviceType type) 
    :  m_DeviceType(type) 
{
}

void GraphicsContext::Clear(uint8_t flags)
{
    GLbitfield mask = 0;
    if (flags & kColorBufferBit)
        mask |= GL_COLOR_BUFFER_BIT;
    if (flags & kDepthBufferBit)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (flags & kStencilBufferBit)
        mask |= GL_STENCIL_BUFFER_BIT;
    if (flags & kAccumBufferBit)
        mask |= GL_ACCUM_BUFFER_BIT;
    glClear(mask);
}

void GraphicsContext::ClearColor(glm::vec4 color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void GraphicsContext::ClearDepth(float depth)
{
    glClearDepthf(depth);
}

void GraphicsContext::SetViewport(int x, int y, size_t width, size_t height)
{
    glViewport(x, y, width, height);
}

void GraphicsContext::SetFrontFace(FrontFaceType flag)
{
    glFrontFace((flag == kCountClockWise) ? GL_CCW : GL_CW);
}

void GraphicsContext::SetCullFace(CullFaceType flag)
{
    glCullFace([&]() {
        if (flag == kCullFront)
            return GL_FRONT;
        else if (flag == kCullBack)
            return GL_BACK;
        return GL_FRONT_AND_BACK;
    }());
}

void GraphicsContext::SetDepthClamp(bool bFlag)
{
    const auto func = bFlag ? glEnable : glDisable;
    func(GL_DEPTH_CLAMP);
}

void GraphicsContext::SetDepthTest(bool bFlag)
{
    const auto func = bFlag ? glEnable : glDisable;
    func(GL_DEPTH_TEST);
}

void GraphicsContext::SetCubemapSeamless(bool bFlag)
{
    const auto func = bFlag ? glEnable : glDisable;
    func(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void GraphicsContext::SetFramebuffer(const GraphicsFramebufferPtr& framebuffer) noexcept
{
    if (!framebuffer)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto fbo = framebuffer->downcast_pointer<OGLCoreFramebuffer>();
        if (fbo) fbo->bind();
    }
    else if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto fbo = framebuffer->downcast_pointer<OGLFramebuffer>();
        if (fbo) fbo->bind();
    }
}

