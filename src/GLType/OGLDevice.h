#pragma once

#include <GLType/GraphicsDevice.h>

class OGLDevice final : public GraphicsDevice
{
    __DeclareSubInterface(OGLDevice, GraphicsDevice)
public:

    OGLDevice() noexcept;
    virtual ~OGLDevice() noexcept;

    bool create(const GraphicsDeviceDesc& desc) noexcept;
    void destoy() noexcept;

    GraphicsDataPtr createGraphicsData(const GraphicsDataDesc& desc) noexcept override;
    GraphicsTexturePtr createTexture(const GraphicsTextureDesc& desc) noexcept override;
    GraphicsFramebufferPtr createFramebuffer(const GraphicsFramebufferDesc& desc) noexcept override;

    void bindRenderTexture(const GraphicsTexturePtr& texture, uint32_t attachment, uint32_t textarget, int32_t level) noexcept override;
    void generateMipmap(const GraphicsTexturePtr& texture) noexcept override;

	const GraphicsDeviceDesc& getGraphicsDeviceDesc() const noexcept override;

private:

    GraphicsDeviceDesc m_Desc;
};
