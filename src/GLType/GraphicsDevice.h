#pragma once

#include <GraphicsTypes.h>

class GraphicsDeviceDesc final
{
public:

    GraphicsDeviceDesc() noexcept;
	~GraphicsDeviceDesc() noexcept;

	void setDeviceType(GraphicsDeviceType type) noexcept;
	GraphicsDeviceType getDeviceType() const noexcept;

private:

    GraphicsDeviceType m_DeviceType;
};

class GraphicsDevice : public rtti::Interface
{
	__DeclareSubInterface(GraphicsDevice, rtti::Interface)
public:

    GraphicsDevice() noexcept;
    virtual ~GraphicsDevice() noexcept;

	virtual GraphicsDataPtr createGraphicsData(const GraphicsDataDesc& desc) noexcept = 0;
    virtual GraphicsTexturePtr createTexture(const GraphicsTextureDesc& desc) noexcept = 0;
    virtual GraphicsFramebufferPtr createFramebuffer(const GraphicsFramebufferDesc& desc) noexcept = 0;

    virtual void setFramebuffer(const GraphicsFramebufferPtr& framebuffer) noexcept = 0;

	virtual const GraphicsDeviceDesc& getGraphicsDeviceDesc() const noexcept = 0;

private:
	GraphicsDevice(const GraphicsDevice&) noexcept = delete;
	GraphicsDevice& operator=(const GraphicsDevice&) noexcept = delete;
};