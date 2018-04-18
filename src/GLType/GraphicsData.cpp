#include <GLType/GraphicsData.h>

__ImplementSubInterface(GraphicsData, rtti::Interface)

GraphicsDataDesc::GraphicsDataDesc() noexcept
	: m_Data(nullptr)
	, m_DataSize(0)
	, m_Usage(GraphicsUsageFlagBits::GraphicsUsageFlagReadBit)
	, m_Type(GraphicsDataType::None)
{
}

GraphicsDataDesc::GraphicsDataDesc(GraphicsDataType type, GraphicsUsageFlags usage, const void* data, uint32_t size) noexcept
	: m_Data((uint8_t*)data)
	, m_DataSize(size)
	, m_Usage(usage)
	, m_Type(type)
{
}

GraphicsDataDesc::~GraphicsDataDesc() noexcept
{
}

void GraphicsDataDesc::setType(GraphicsDataType type) noexcept
{
    m_Type = type;
}

GraphicsDataType GraphicsDataDesc::getType() const noexcept
{
    return m_Type;
}

void GraphicsDataDesc::setUsage(GraphicsUsageFlags usage) noexcept
{
    m_Usage = usage;
}

GraphicsUsageFlags GraphicsDataDesc::getUsage() const noexcept
{
    return m_Usage;
}

void GraphicsDataDesc::setStream(std::uint8_t* data) noexcept
{
    m_Data = data;
}

std::uint8_t* GraphicsDataDesc::getStream() const noexcept
{
    return m_Data;
}

void GraphicsDataDesc::setStreamSize(std::uint32_t size) noexcept
{
    m_DataSize = size;;
}

std::uint32_t GraphicsDataDesc::getStreamSize() const noexcept
{
    return m_DataSize;
}

GraphicsData::GraphicsData() noexcept
{
}

GraphicsData::~GraphicsData() noexcept
{
}
