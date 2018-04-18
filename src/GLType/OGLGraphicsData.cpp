#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLTypes.h>
#include <cassert>

__ImplementSubInterface(OGLGraphicsData, GraphicsData)

OGLGraphicsData::OGLGraphicsData() noexcept
    : m_BufferID(GL_NONE)
    , m_Target(GL_NONE)
{
}

OGLGraphicsData::~OGLGraphicsData() noexcept
{
    destroy();
}

bool OGLGraphicsData::create(const GraphicsDataDesc& desc) noexcept
{
	assert(m_BufferID == GL_NONE);
    assert(desc.getStreamSize() > 0);
    if (m_BufferID != GL_NONE)
        return false;

    // TODO: Add other type
    auto type = desc.getType();
    if (type == GraphicsDataType::UniformBuffer)
        m_Target =  GL_UNIFORM_BUFFER;
    else
    {
        // TODO: add handler for false case
        assert(false);
		printf("Not supporting graphics data types.");
        return false;
    }

    // https://stackoverflow.com/a/8281825/1890382
    GLenum flag = GL_STATIC_DRAW;
    auto usage = desc.getUsage();
    if (usage & GraphicsUsageFlagWriteBit)
        flag = GL_DYNAMIC_DRAW;

	glGenBuffers(1, &m_BufferID);
	glBindBuffer(m_Target, m_BufferID);
	glBufferData(m_Target, desc.getStreamSize(), desc.getStream(), flag);

	if (m_BufferID == GL_NONE)
	{
		printf("glGenBuffers() fail.");
		return false;
	}

	m_Desc = desc;
    return true;
}

void OGLGraphicsData::destroy() noexcept
{
    m_Target = GL_NONE;
    if (m_BufferID)
    {
        glDeleteBuffers(1, &m_BufferID);
        m_BufferID = GL_NONE;
    }
}

bool OGLGraphicsData::map(std::ptrdiff_t offset, std::ptrdiff_t count, void** data, GraphicsUsageFlags flags) noexcept
{
	assert(data);
	glBindBuffer(m_Target, m_BufferID);
	*data = glMapBufferRange(m_Target, offset, count, OGLTypes::translate(flags));
	return *data ? true : false;
}

void OGLGraphicsData::unmap() noexcept
{
    glBindBuffer(m_Target, m_BufferID);
    glUnmapBuffer(m_Target);
}

void OGLGraphicsData::update(std::ptrdiff_t offset, std::ptrdiff_t count, void* data)
{
    glBindBuffer(m_Target, m_BufferID);
    glBufferSubData(m_Target, offset, count, data);
}

GLuint OGLGraphicsData::getInstanceID() const noexcept
{
    assert(m_BufferID != GL_NONE);
    return m_BufferID;
}

void OGLGraphicsData::setDevice(GraphicsDevicePtr device) noexcept
{
    m_Device = device;
}

GraphicsDevicePtr OGLGraphicsData::getDevice() noexcept
{
    return m_Device.lock();
}
