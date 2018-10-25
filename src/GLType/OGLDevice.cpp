#include <GLType/OGLDevice.h>
#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLCoreGraphicsData.h>
#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>
#include <GLType/OGLFramebuffer.h>
#include <GLType/OGLCoreFramebuffer.h>

__ImplementSubInterface(OGLDevice, GraphicsDevice)

OGLDevice::OGLDevice() noexcept
{
}

OGLDevice::~OGLDevice() noexcept
{
}

bool OGLDevice::create(const GraphicsDeviceDesc& desc) noexcept
{
    m_Desc = desc;
    return true;
}

void OGLDevice::destoy() noexcept
{
}

GraphicsDataPtr OGLDevice::createGraphicsData(const GraphicsDataDesc& desc) noexcept
{
    if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto data = std::make_shared<OGLCoreGraphicsData>();
        if (!data) return nullptr;
		data->setDevice(this->downcast_pointer<OGLDevice>());
        if (data->create(desc))
            return data;
        return nullptr;
    }
    else if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto data = std::make_shared<OGLGraphicsData>();
        if (!data) return nullptr;
		data->setDevice(this->downcast_pointer<OGLDevice>());
        if (data->create(desc))
            return data;
        return nullptr;
    }
    return nullptr;
}

GraphicsTexturePtr OGLDevice::createTexture(const GraphicsTextureDesc& desc) noexcept
{
    assert(desc.getWrapS() != GL_CLAMP);
    assert(desc.getWrapT() != GL_CLAMP);
    assert(desc.getWrapR() != GL_CLAMP);
    assert(desc.getMagFilter() == GL_NEAREST || desc.getMagFilter() == GL_LINEAR);

    if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto texture = std::make_shared<OGLCoreTexture>();
        if (!texture) return nullptr;
		texture->setDevice(this->downcast_pointer<OGLDevice>());
        if (texture->create(desc))
            return texture;
        return nullptr;
    }
    else if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto texture = std::make_shared<OGLTexture>();
        if (!texture) return nullptr;
		texture->setDevice(this->downcast_pointer<OGLDevice>());
        if (texture->create(desc))
            return texture;
        return nullptr;
    }
    return nullptr;
}

GraphicsFramebufferPtr OGLDevice::createFramebuffer(const GraphicsFramebufferDesc& desc) noexcept
{
    if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto fbo = std::make_shared<OGLCoreFramebuffer>();
        if (!fbo) return nullptr;
		fbo->setDevice(this->downcast_pointer<OGLDevice>());
        if (fbo->create(desc))
            return fbo;
        return nullptr;
    }
    else if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto fbo = std::make_shared<OGLFramebuffer>();
        if (!fbo) return nullptr;
		fbo->setDevice(this->downcast_pointer<OGLDevice>());
        if (fbo->create(desc))
            return fbo;
        return nullptr;
    }
    return nullptr;
}

void OGLDevice::bindRenderTexture(const GraphicsTexturePtr& texture, uint32_t attachment, uint32_t textarget, int32_t level) noexcept 
{
    if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto tex = texture->downcast_pointer<OGLCoreTexture>();
        if (tex) glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textarget, tex->getTextureID(), level);
    }
    else if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto tex = texture->downcast_pointer<OGLTexture>();
        if (tex) glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textarget, tex->getTextureID(), level);
    }
}

void OGLDevice::generateMipmap(const GraphicsTexturePtr& texture) noexcept
{
    if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto tex = texture->downcast_pointer<OGLCoreTexture>();
        if (tex) tex->generateMipmap();
    }
    else if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto tex = texture->downcast_pointer<OGLTexture>();
        if (tex) tex->generateMipmap();
    }
}

const GraphicsDeviceDesc& OGLDevice::getGraphicsDeviceDesc() const noexcept
{
    return m_Desc;
}
