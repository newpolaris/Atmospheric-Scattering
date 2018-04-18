#pragma once

#include <GL/glew.h>
#include <GLType/GraphicsData.h>

class OGLGraphicsData final : public GraphicsData
{
    __DeclareSubInterface(OGLGraphicsData, GraphicsData)
public:

    OGLGraphicsData() noexcept;
	virtual ~OGLGraphicsData() noexcept;

    bool create(const GraphicsDataDesc& desc) noexcept; 
	void destroy() noexcept;

	virtual bool map(std::ptrdiff_t offset, std::ptrdiff_t count, void** data, GraphicsUsageFlags flags) noexcept override;
	virtual void unmap() noexcept override;

    virtual void update(std::ptrdiff_t offset, std::ptrdiff_t count, void* data) override;

	GLuint getInstanceID() const noexcept;

private:

	friend class OGLDevice;
	void setDevice(GraphicsDevicePtr device) noexcept;
	GraphicsDevicePtr getDevice() noexcept;

private:

	OGLGraphicsData(const OGLGraphicsData&) noexcept = delete;
	OGLGraphicsData& operator=(const OGLGraphicsData&) noexcept = delete;

private:

	GLenum m_Target;
    GLuint m_BufferID;
	GraphicsDataDesc m_Desc;
	GraphicsDeviceWeakPtr m_Device;
};
