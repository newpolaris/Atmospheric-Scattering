#include "GLType/OGLFramebuffer.h"
#include <GL/glew.h>
#include <GLType/OGLTexture.h>
#include <cassert>

__ImplementSubInterface(OGLFramebuffer, GraphicsFramebuffer)

OGLFramebuffer::OGLFramebuffer() noexcept 
    : m_FBO(GL_NONE)
{
}

OGLFramebuffer::~OGLFramebuffer() noexcept
{
    destroy();
}

void OGLFramebuffer::bind() noexcept
{
    assert(m_FBO != GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

void OGLFramebuffer::setDevice(GraphicsDevicePtr device) noexcept
{
    m_Device = device;
}

GraphicsDevicePtr OGLFramebuffer::getDevice() noexcept
{
    return m_Device.lock();
}

bool OGLFramebuffer::create(const GraphicsFramebufferDesc& desc) noexcept
{
    assert(m_FBO == GL_NONE);

    m_FramebufferDesc = desc;

    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	GLsizei drawCount = 0;
    GLenum drawBuffers[GL_COLOR_ATTACHMENT15 - GL_COLOR_ATTACHMENT0] = { GL_NONE, };

    const auto& components = desc.getComponents();
    for (const auto& c : components)
    {
        auto attachment = c.getAttachment();
        auto texture = c.getTexture()->downcast_pointer<OGLTexture>();
        auto levels = c.getMipLevel();
        auto layer = c.getLayer();
        if (layer > 0)
            glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture->getTextureID(), levels, layer);
        else
            glFramebufferTexture(GL_FRAMEBUFFER,  attachment, texture->getTextureID(), levels);

        if (attachment != GL_DEPTH_ATTACHMENT)
            drawBuffers[drawCount++] = attachment;
    }
    glDrawBuffers(drawCount, drawBuffers);

    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void OGLFramebuffer::destroy() noexcept
{
    if (m_FBO != GL_NONE)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

const GraphicsFramebufferDesc& OGLFramebuffer::getGraphicsFramebufferDesc() const noexcept
{
    return m_FramebufferDesc;
}