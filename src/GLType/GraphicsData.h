#pragma once

#include <GraphicsTypes.h>

class GraphicsDataDesc final
{
public:

    GraphicsDataDesc() noexcept;
	GraphicsDataDesc(GraphicsDataType type, GraphicsUsageFlags usage, const void* data, uint32_t size) noexcept;
	~GraphicsDataDesc() noexcept;

	void setType(GraphicsDataType type) noexcept;
	GraphicsDataType getType() const noexcept;

	void setUsage(GraphicsUsageFlags usage) noexcept;
	GraphicsUsageFlags getUsage() const noexcept;

	void setStream(std::uint8_t* data) noexcept;
	std::uint8_t* getStream() const noexcept;

	void setStreamSize(std::uint32_t size) noexcept;
	std::uint32_t getStreamSize() const noexcept;

private:

	std::uint8_t* m_Data;
	std::uint32_t m_DataSize;
	GraphicsUsageFlags m_Usage;
	GraphicsDataType m_Type;
};

class GraphicsData : public rtti::Interface
{
    __DeclareSubInterface(GraphicsData, rtti::Interface)
public:

    GraphicsData() noexcept;
    virtual ~GraphicsData() noexcept;

	virtual bool map(std::ptrdiff_t offset, std::ptrdiff_t count, void** data, GraphicsUsageFlags flags) noexcept = 0;
	virtual void unmap() noexcept = 0;

    virtual void update(std::ptrdiff_t offset, std::ptrdiff_t count, void* data) = 0;

private:

	GraphicsData(const GraphicsData&) = delete;
	GraphicsData& operator=(const GraphicsData&) = delete;

};
