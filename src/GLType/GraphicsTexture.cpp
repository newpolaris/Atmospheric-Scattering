#include <GLType/GraphicsTexture.h>
#include <GL/glew.h>

__ImplementSubInterface(GraphicsTexture, rtti::Interface)

GraphicsTextureDesc::GraphicsTextureDesc() noexcept
    : m_Width(1)
    , m_Height(1)
    , m_Depth(1)
    , m_Levels(1)
    , m_WrapS(GL_REPEAT)
    , m_WrapT(GL_REPEAT)
    , m_WrapR(GL_REPEAT)
    , m_MinFilter(GL_NEAREST_MIPMAP_LINEAR)
    , m_MagFilter(GL_LINEAR)
    , m_AnisotropyLevel(0)
    , m_Target(gli::TARGET_2D)
    , m_Format(gli::FORMAT_UNDEFINED)
    , m_Data(nullptr)
	, m_DataSize(0)
{
}

GraphicsTextureDesc::~GraphicsTextureDesc() noexcept
{
}

std::string GraphicsTextureDesc::getName() const noexcept
{
    return m_Name;
}

void GraphicsTextureDesc::setName(const std::string& name) noexcept
{
    m_Name = name;
}

std::string GraphicsTextureDesc::getFileName() const noexcept
{
    return m_Filename;
}

void GraphicsTextureDesc::setFilename(const std::string& filename) noexcept
{
    m_Filename = filename;
}

int32_t GraphicsTextureDesc::getWidth() const noexcept
{
    return m_Width;
}

void GraphicsTextureDesc::setWidth(int32_t width) noexcept
{
    m_Width = width;
}

int32_t GraphicsTextureDesc::getHeight() const noexcept
{
    return m_Height;
}

void GraphicsTextureDesc::setHeight(int32_t height) noexcept
{
    m_Height = height;
}

int32_t GraphicsTextureDesc::getDepth() const noexcept
{
    return m_Depth;
}

void GraphicsTextureDesc::setDepth(int32_t depth) noexcept
{
    m_Depth = depth;
}

int32_t GraphicsTextureDesc::getLevels() const noexcept
{
    return m_Levels;
}

void GraphicsTextureDesc::setLevels(int32_t levels) noexcept
{
    m_Levels = levels;
}

std::uint8_t* GraphicsTextureDesc::getStream() const noexcept
{
    return m_Data;
}

void GraphicsTextureDesc::setStream(std::uint8_t* data) noexcept
{
    m_Data = data;
}

std::uint32_t GraphicsTextureDesc::getStreamSize() const noexcept
{
    return m_DataSize;
}

void GraphicsTextureDesc::setStreamSize(std::uint32_t size) noexcept
{
    m_DataSize = size;
}

GraphicsTarget GraphicsTextureDesc::getTarget() const noexcept
{
    return m_Target;
}

void GraphicsTextureDesc::setTarget(GraphicsTarget target) noexcept
{
    m_Target = target;
}

GraphicsFormat GraphicsTextureDesc::getFormat() const noexcept
{
    return m_Format;
}

void GraphicsTextureDesc::setFormat(GraphicsFormat format) noexcept
{
    m_Format = format;
}

uint32_t GraphicsTextureDesc::getWrapS() const noexcept
{
    return m_WrapS;
}

void GraphicsTextureDesc::setWrapS(uint32_t wrap) noexcept
{
    m_WrapS = wrap;
}

uint32_t GraphicsTextureDesc::getWrapT() const noexcept
{
    return m_WrapT;
}

void GraphicsTextureDesc::setWrapT(uint32_t wrap) noexcept
{
    m_WrapT = wrap;
}

uint32_t GraphicsTextureDesc::getWrapR() const noexcept
{
    return m_WrapT;
}

void GraphicsTextureDesc::setWrapR(uint32_t wrap) noexcept
{
    m_WrapR = wrap;
}

uint32_t GraphicsTextureDesc::getMinFilter() const noexcept
{
    return m_MinFilter;
}

void GraphicsTextureDesc::setMinFilter(uint32_t filter) noexcept
{
    m_MinFilter = filter;
}

uint32_t GraphicsTextureDesc::getMagFilter() const noexcept
{
    return m_MagFilter;
}

void GraphicsTextureDesc::setMagFilter(uint32_t filter) noexcept
{
    m_MagFilter = filter;
}

float GraphicsTextureDesc::getAnisotropyLevel() const noexcept
{
    return m_AnisotropyLevel;
}

void GraphicsTextureDesc::setAnisotropyLevel(float anisoLevel) noexcept
{
    m_AnisotropyLevel = anisoLevel;
}

GraphicsTexture::GraphicsTexture() noexcept
{
}

GraphicsTexture::~GraphicsTexture() noexcept
{
}
