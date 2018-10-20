#include "GLType/OGLCoreFramebuffer.h"
#include <GL/glew.h>
#include <GLType/OGLCoreTexture.h>
#include <cassert>

__ImplementSubInterface(OGLCoreFramebuffer, GraphicsFramebuffer)

OGLCoreFramebuffer::OGLCoreFramebuffer() noexcept 
    : m_FBO(GL_NONE)
{
}

OGLCoreFramebuffer::~OGLCoreFramebuffer() noexcept
{
    destroy();
}

void OGLCoreFramebuffer::bind() noexcept
{
    assert(m_FBO != GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

void OGLCoreFramebuffer::setDevice(GraphicsDevicePtr device) noexcept
{
    m_Device = device;
}

GraphicsDevicePtr OGLCoreFramebuffer::getDevice() noexcept
{
    return m_Device.lock();
}

bool OGLCoreFramebuffer::create(const GraphicsFramebufferDesc& desc) noexcept
{
    assert(m_FBO == GL_NONE);

    m_FramebufferDesc = desc;

    glCreateFramebuffers(1, &m_FBO);

	GLsizei drawCount = 0;
    GLenum drawBuffers[GL_COLOR_ATTACHMENT15 - GL_COLOR_ATTACHMENT0] = { GL_NONE, };

    const auto& components = desc.getComponents();
    for (const auto& c : components)
    {
        auto attachment = c.getAttachment();
        auto texture = c.getTexture()->downcast_pointer<OGLCoreTexture>();
        auto levels = c.getMipLevel();
        auto layer = c.getLayer();
        if (layer > 0)
            glNamedFramebufferTextureLayer(m_FBO, attachment, texture->getTextureID(), levels, layer);
        else
            glNamedFramebufferTexture(m_FBO, attachment, texture->getTextureID(), levels);

        if (attachment != GL_DEPTH_ATTACHMENT && attachment != GL_DEPTH_STENCIL_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT)
            drawBuffers[drawCount++] = attachment;
    }
    glNamedFramebufferDrawBuffers(m_FBO, drawCount, drawBuffers);

    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void OGLCoreFramebuffer::destroy() noexcept
{
    if (m_FBO != GL_NONE)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

const GraphicsFramebufferDesc& OGLCoreFramebuffer::getGraphicsFramebufferDesc() const noexcept
{
    return m_FramebufferDesc;
}
