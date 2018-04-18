#pragma once

#include <GL/glew.h>
#include <GLType/GraphicsData.h>

class OGLCoreGraphicsData final : public GraphicsData
{
    __DeclareSubInterface(OGLCoreGraphicsData, GraphicsData)
public:

    OGLCoreGraphicsData() noexcept;
	virtual ~OGLCoreGraphicsData() noexcept;

    bool create(const GraphicsDataDesc& desc) noexcept; 
	void destroy() noexcept;

	virtual bool map(std::ptrdiff_t offset, std::ptrdiff_t count, void** data, GraphicsUsageFlags flags) noexcept override;
	virtual void unmap() noexcept override;

    int flush() noexcept;
    int flush(GLintptr offset, GLsizeiptr cnt) noexcept;

    virtual void update(std::ptrdiff_t offset, std::ptrdiff_t count, void* data) override;

	GLuint getInstanceID() const noexcept;

private:

	friend class OGLDevice;
	void setDevice(GraphicsDevicePtr device) noexcept;
	GraphicsDevicePtr getDevice() noexcept;

private:

	OGLCoreGraphicsData(const OGLCoreGraphicsData&) noexcept = delete;
	OGLCoreGraphicsData& operator=(const OGLCoreGraphicsData&) noexcept = delete;

private:

    GLuint m_BufferID;
	GraphicsDataDesc m_Desc;
	GraphicsDeviceWeakPtr m_Device;
};
