#include "GLType/GraphicsFramebuffer.h"

__ImplementSubInterface(GraphicsFramebuffer, rtti::Interface)

GraphicsFramebufferDesc::GraphicsFramebufferDesc() noexcept
{
}

GraphicsFramebufferDesc::~GraphicsFramebufferDesc() noexcept
{
}

void GraphicsFramebufferDesc::addComponent(const GraphicsAttachmentBinding& component) noexcept
{
    m_Bindings.push_back(component);
}

const AttachmentBindings& GraphicsFramebufferDesc::getComponents() const noexcept
{
    return m_Bindings;
}

GraphicsAttachmentBinding::GraphicsAttachmentBinding(const GraphicsTexturePtr& texture, std::uint32_t attachment, std::uint32_t mipLevel, std::uint32_t layer) noexcept
    : m_Texture(texture)
    , m_Attachment(attachment)
    , m_MipLevel(mipLevel)
    , m_Layer(layer)
{
}

GraphicsAttachmentBinding::~GraphicsAttachmentBinding() noexcept
{
}

GraphicsTexturePtr GraphicsAttachmentBinding::getTexture() const noexcept
{
    return m_Texture;
}

void GraphicsAttachmentBinding::setTexture(const GraphicsTexturePtr& texture) noexcept
{
    m_Texture = texture;
}

std::uint32_t GraphicsAttachmentBinding::getAttachment() const noexcept
{
    return m_Attachment;
}

void GraphicsAttachmentBinding::setAttachment(std::uint32_t attachment) noexcept
{
    m_Attachment = attachment;
}

std::uint32_t GraphicsAttachmentBinding::getMipLevel() const noexcept
{
    return m_MipLevel;
}

void GraphicsAttachmentBinding::setMipLevel(std::uint32_t mipLevel) noexcept
{
    m_MipLevel = mipLevel;
}

std::uint32_t GraphicsAttachmentBinding::getLayer() const noexcept
{
    return m_Layer;
}

void GraphicsAttachmentBinding::setLayer(std::uint32_t layer) noexcept
{
    m_Layer = layer;
}

GraphicsFramebuffer::GraphicsFramebuffer() noexcept
{
}

GraphicsFramebuffer::~GraphicsFramebuffer() noexcept
{
}