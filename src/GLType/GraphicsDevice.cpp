#include <GLType/GraphicsDevice.h>

__ImplementSubInterface(GraphicsDevice, rtti::Interface)

GraphicsDevice::GraphicsDevice() noexcept
{
}

GraphicsDevice::~GraphicsDevice() noexcept
{
}

GraphicsDeviceDesc::GraphicsDeviceDesc() noexcept
    : m_DeviceType(GraphicsDeviceTypeMaxEnum)
{
}

GraphicsDeviceDesc::~GraphicsDeviceDesc() noexcept
{
}

void GraphicsDeviceDesc::setDeviceType(GraphicsDeviceType type) noexcept
{
    m_DeviceType = type;
}

GraphicsDeviceType GraphicsDeviceDesc::getDeviceType() const noexcept
{
    return m_DeviceType;
}
