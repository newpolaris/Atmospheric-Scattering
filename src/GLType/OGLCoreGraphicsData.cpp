#include <GLType/OGLCoreGraphicsData.h>
#include <GLType/OGLTypes.h>
#include <cassert>

__ImplementSubInterface(OGLCoreGraphicsData, GraphicsData)

OGLCoreGraphicsData::OGLCoreGraphicsData() noexcept 
    : m_BufferID(GL_NONE)
{
}

OGLCoreGraphicsData::~OGLCoreGraphicsData() noexcept
{
    destroy();
}

bool OGLCoreGraphicsData::create(const GraphicsDataDesc& desc) noexcept
{
	assert(m_BufferID == GL_NONE);
    assert(desc.getStreamSize() > 0);

    glCreateBuffers(1, &m_BufferID);
	if (m_BufferID == GL_NONE)
	{
		printf("glCreateBuffers() fail.");
		return false;
	}

    GLbitfield flags = OGLTypes::translate(desc.getUsage());
    glNamedBufferStorage(m_BufferID, desc.getStreamSize(), desc.getStream(), flags);

    // TODO: check bufer can handle vertex/index buffer without setting.

	m_Desc = desc;
    return true;
}

void OGLCoreGraphicsData::destroy() noexcept
{
    if (m_BufferID)
    {
        glDeleteBuffers(1, &m_BufferID);
        m_BufferID = GL_NONE;
    }
}

bool OGLCoreGraphicsData::map(std::ptrdiff_t offset, std::ptrdiff_t count, void** data, GraphicsUsageFlags flags) noexcept
{
	assert(data);
	*data = glMapNamedBufferRange(m_BufferID, offset, count, OGLTypes::translate(flags));
	return *data ? true : false;
    return false;
}

void OGLCoreGraphicsData::unmap() noexcept
{
	glUnmapNamedBuffer(m_BufferID);
}

int OGLCoreGraphicsData::flush() noexcept
{
	return this->flush(0, m_Desc.getStreamSize());
}

int OGLCoreGraphicsData::flush(GLintptr offset, GLsizeiptr cnt) noexcept
{
	glFlushMappedNamedBufferRange(m_BufferID, offset, cnt);
	return cnt;
}

void OGLCoreGraphicsData::update(std::ptrdiff_t offset, std::ptrdiff_t count, void* data)
{
    glNamedBufferSubData(m_BufferID, offset, count, data);
}

GLuint OGLCoreGraphicsData::getInstanceID() const noexcept
{
    assert(m_BufferID != GL_NONE);
    return m_BufferID;
}

void OGLCoreGraphicsData::setDevice(GraphicsDevicePtr device) noexcept
{
    m_Device = device;
}

GraphicsDevicePtr OGLCoreGraphicsData::getDevice() noexcept
{
    return m_Device.lock();
}
