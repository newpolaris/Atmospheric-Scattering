#pragma once

#include <GraphicsTypes.h>
#include <tools/Rtti.h>

class GraphicsAttachmentBinding final
{
public:

    GraphicsAttachmentBinding(const GraphicsTexturePtr& texture, std::uint32_t attachment, std::uint32_t mipLevel = 0, std::uint32_t layer = 0) noexcept;
    ~GraphicsAttachmentBinding() noexcept;

    GraphicsTexturePtr getTexture() const noexcept;
    void setTexture(const GraphicsTexturePtr& texture) noexcept;

    std::uint32_t getAttachment() const noexcept;
    void setAttachment(std::uint32_t attachment) noexcept;

    std::uint32_t getMipLevel() const noexcept;
    void setMipLevel(std::uint32_t mipLevel) noexcept;

    std::uint32_t getLayer() const noexcept;
    void setLayer(std::uint32_t layer) noexcept;

    std::uint32_t m_Attachment;
    std::uint32_t m_MipLevel;
    std::uint32_t m_Layer;
    GraphicsTexturePtr m_Texture;
};

class GraphicsFramebufferDesc final
{
public:

    GraphicsFramebufferDesc() noexcept;
    ~GraphicsFramebufferDesc() noexcept;

	void addComponent(const GraphicsAttachmentBinding& component) noexcept;
	const AttachmentBindings& getComponents() const noexcept;

private:

	AttachmentBindings m_Bindings;
};

class GraphicsFramebuffer : public rtti::Interface
{
    __DeclareSubInterface(GraphicsFramebuffer, rtti::Interface)
public:

    GraphicsFramebuffer() noexcept;
    virtual ~GraphicsFramebuffer() noexcept;

    virtual const GraphicsFramebufferDesc& getGraphicsFramebufferDesc() const noexcept = 0;
};
