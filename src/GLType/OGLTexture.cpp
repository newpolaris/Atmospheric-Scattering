#include <gli/gli.hpp>
#include <tools/stb_image.h>
#include <tools/string.h>
#include <tools/FileUtility.h>
#include <GLType/OGLTypes.h>
#include <GLType/OGLTexture.h>

__ImplementSubInterface(OGLTexture, GraphicsTexture)

OGLTexture::OGLTexture()
    : m_TextureID(GL_NONE)
    , m_Target(GL_INVALID_ENUM)
    , m_Format(GL_INVALID_ENUM)
	, m_PBO(GL_NONE)
	, m_PBOSize(0)
{
}

OGLTexture::~OGLTexture()
{
	destroy();
}

bool OGLTexture::create(GLint width, GLint height, GLenum target, GraphicsFormat format, GLuint levels, const uint8_t* data, uint32_t size) noexcept
{
    using namespace gli;

    const gl GL(gl::PROFILE_GL33);
    const swizzles swizzle(gl::SWIZZLE_RED, gl::SWIZZLE_GREEN, gl::SWIZZLE_BLUE, gl::SWIZZLE_ALPHA);
    const auto Format = GL.translate(format, swizzle);

	GLuint TextureID = 0;
	glGenTextures(1, &TextureID);
	glBindTexture(target, TextureID);
    glTexStorage2D(target, levels, Format.Internal, width, height);
    if (data != nullptr && size != 0)
    {
        if (gli::is_compressed(format))
            glCompressedTexSubImage2D(target, 0, 0, 0, width, height, Format.External, size, data);
        else
            glTexSubImage2D(target, 0, 0, 0, width, height, Format.External, Format.Type, data);
	}

	m_Target = target;
	m_TextureID = TextureID;
	m_Format = format;

	return true;
}

bool OGLTexture::create(const GraphicsTextureDesc& desc) noexcept
{
    m_TextureDesc = desc;

    bool bSuccess = false;
    auto filename = desc.getFileName();
    if (!filename.empty())
        bSuccess = create(filename);
    else
    {
        auto width = desc.getWidth();
        auto height = desc.getHeight();
        auto levels = desc.getLevels();
        auto format = desc.getFormat();
        auto data = desc.getStream();
        auto size = desc.getStreamSize();
        auto target = OGLTypes::translate(desc.getTarget());
        bSuccess = create(width, height, target, format, levels, data, size);
    }
    if (bSuccess) applyParameters(desc);
    return bSuccess;
}

bool OGLTexture::create(const std::string& filename) noexcept
{
    static_assert(std::is_same<char, std::istream::char_type>::value, "Compatible type needed");

    if (filename.empty()) 
        return false;

    auto data = util::ReadFileSync(filename);
    if (data == util::NullFile)
        return false;

    const std::string ext = util::getFileExtension(filename);
    if (util::stricmp(ext, "zlib"))
        return createFromMemoryZIP(data->data(), data->size());
    else if (util::stricmp(ext, "DDS") || util::stricmp(ext, "KTX"))
        return createFromMemoryDDS(data->data(), data->size());
    else if (util::stricmp(ext, "HDR"))
        return createFromMemoryHDR(data->data(), data->size());
    return createFromMemoryLDR(data->data(), data->size());
} 

bool OGLTexture::createFromMemoryLDR(const char* data, size_t size) noexcept
{
    stbi_set_flip_vertically_on_load(true);

	GLenum target = GL_TEXTURE_2D;
    GLenum type = GL_UNSIGNED_BYTE;
    int width = 0, height = 0, nrComponents = 0;
    stbi_uc* imagedata = stbi_load_from_memory((stbi_uc*)data, (int)size, &width, &height, &nrComponents, 0);
    if (!imagedata) return false;

    GLenum Format = OGLTypes::getComponent(nrComponents);
    GLenum InternalFormat = OGLTypes::getInternalComponent(nrComponents, type == GL_FLOAT);

	gli::gl GL(gli::gl::PROFILE_GL33);
    gli::format format = GL.find(
        static_cast<gli::gl::internal_format>(InternalFormat),
        static_cast<gli::gl::external_format>(Format),
        static_cast<gli::gl::type_format>(type));

    bool bSuccess = create(width, height, target, format, 1, (const uint8_t*)imagedata, size);
    stbi_image_free(imagedata);
    return bSuccess;
}

bool OGLTexture::createFromMemoryZIP(const char* data, size_t dataSize) noexcept
{
    int decodesize = 0;
    // malloc never throws exception
    char* decodedata = stbi_zlib_decode_malloc(data, (int)dataSize, &decodesize);
    if (decodedata == nullptr)
        return false;
    bool bSuccess = createFromMemory(decodedata, decodesize);
    free(decodedata);
    return bSuccess;
}

GLuint OGLTexture::getTextureID() const noexcept
{
    return m_TextureID;
}

GLenum OGLTexture::getFormat() const noexcept
{
    return m_Format;
}

const GraphicsTextureDesc& OGLTexture::getGraphicsTextureDesc() const noexcept
{
    return m_TextureDesc;
}

void OGLTexture::setDevice(const GraphicsDevicePtr& device) noexcept
{
    m_Device = device;
}

GraphicsDevicePtr OGLTexture::getDevice() noexcept
{
    return m_Device.lock();
}

void OGLTexture::destroy() noexcept
{
	if (m_TextureID != GL_NONE)
	{
		glDeleteTextures(1, &m_TextureID);
		m_TextureID = GL_NONE;

        m_Format = GL_INVALID_ENUM;
	}
	m_Target = GL_INVALID_ENUM;

	if (m_PBO != GL_NONE)
	{
		glDeleteBuffers(1, &m_PBO);
		m_PBO = GL_NONE;
		m_PBOSize = 0;
	}
}

void OGLTexture::bind(GLuint unit) const
{
	assert( 0u != m_TextureID );  
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(m_Target, m_TextureID);
}

void OGLTexture::unbind(GLuint unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(m_Target, 0u);
}

void OGLTexture::generateMipmap()
{
	assert(m_Target != GL_INVALID_ENUM);
	assert(m_TextureID != 0);

	glBindTexture(m_Target, m_TextureID);
	glGenerateMipmap(m_Target);
}

void OGLTexture::applyParameters(const GraphicsTextureDesc& desc)
{
    auto wrapS = desc.getWrapS();
    auto wrapT = desc.getWrapT();
    auto wrapR = desc.getWrapR();
    auto defaultWrap = GL_REPEAT;

    if (wrapS != defaultWrap)
        parameteri(GL_TEXTURE_WRAP_S, wrapS);
    if (wrapT != defaultWrap)
        parameteri(GL_TEXTURE_WRAP_T, wrapS);
    if (wrapR != defaultWrap)
        parameteri(GL_TEXTURE_WRAP_R, wrapR);

    auto minFilter = desc.getMinFilter();
    auto magFilter = desc.getMagFilter();
    auto defaultMinFilter = GL_LINEAR_MIPMAP_LINEAR;
    auto defaultMagFilter = GL_LINEAR;
    assert(magFilter == GL_NEAREST || magFilter == GL_LINEAR);
    if (minFilter != defaultMinFilter)
        parameteri(GL_TEXTURE_MIN_FILTER, minFilter);
    if (magFilter != defaultMagFilter)
        parameteri(GL_TEXTURE_MAG_FILTER, magFilter);

    GLfloat defaultAnisoLevel = 0;
    auto anisoLevel = desc.getAnisotropyLevel();
    if (anisoLevel > defaultAnisoLevel)
        parameterf(GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoLevel);
}


void OGLTexture::parameteri(GLenum pname, GLint param)
{
	assert(m_Target != GL_INVALID_ENUM);
	assert(m_TextureID != 0);

	glBindTexture(m_Target, m_TextureID);
    glTexParameteri(m_Target, pname, param);
}

void OGLTexture::parameterf(GLenum pname, GLfloat param)
{
	assert(m_Target != GL_INVALID_ENUM);
	assert(m_TextureID != 0);

	glBindTexture(m_Target, m_TextureID);
    glTexParameterf(m_Target, pname, param);
}

bool OGLTexture::createFromMemory(const char* data, size_t dataSize) noexcept
{
    if (createFromMemoryZIP(data, dataSize)
        || createFromMemoryDDS(data, dataSize)
        || createFromMemoryLDR(data, dataSize)
        || createFromMemoryHDR(data, dataSize))
        return true;
    return false;
}

bool OGLTexture::createFromMemoryDDS(const char* data, size_t dataSize) noexcept
{
	gli::texture Texture = gli::load(data, dataSize);
	if (Texture.empty())
		return false;

    Texture = gli::flip(Texture);

	gli::gl GL(gli::gl::PROFILE_GL33);
	gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
	GLenum Target = GL.translate(Texture.target());

	GLuint TextureID = 0;
	glGenTextures(1, &TextureID);
	glBindTexture(Target, TextureID);
	glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);

	glm::tvec3<GLsizei> const Extent(Texture.extent());
	GLsizei const FaceTotal = static_cast<GLsizei>(Texture.layers() * Texture.faces());

	switch(Texture.target())
	{
	case gli::TARGET_1D:
		glTexStorage1D(
			Target, static_cast<GLint>(Texture.levels()), Format.Internal, Extent.x);
		break;
	case gli::TARGET_1D_ARRAY:
	case gli::TARGET_2D:
	case gli::TARGET_CUBE:
		glTexStorage2D(
			Target, static_cast<GLint>(Texture.levels()), Format.Internal,
			Extent.x, Extent.y);
		break;
	case gli::TARGET_2D_ARRAY:
	case gli::TARGET_3D:
	case gli::TARGET_CUBE_ARRAY:
		glTexStorage3D(
			Target, static_cast<GLint>(Texture.levels()), Format.Internal,
			Extent.x, Extent.y,
			Texture.target() == gli::TARGET_3D ? Extent.z : FaceTotal);
		break;
	default:
		assert(0);
		break;
	}

	for(std::size_t Layer = 0; Layer < Texture.layers(); ++Layer)
	for(std::size_t Face = 0; Face < Texture.faces(); ++Face)
	for(std::size_t Level = 0; Level < Texture.levels(); ++Level)
	{
		GLsizei const LayerGL = static_cast<GLsizei>(Layer);
		glm::tvec3<GLsizei> Extent(Texture.extent(Level));
		GLenum _Target = gli::is_target_cube(Texture.target())
			? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face)
			: Target;

		switch(Texture.target())
		{
		case gli::TARGET_1D:
			if(gli::is_compressed(Texture.format()))
				glCompressedTexSubImage1D(
					_Target, static_cast<GLint>(Level), 0, Extent.x,
					Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
					Texture.data(Layer, Face, Level));
			else
				glTexSubImage1D(
					_Target, static_cast<GLint>(Level), 0, Extent.x,
					Format.External, Format.Type,
					Texture.data(Layer, Face, Level));
			break;
		case gli::TARGET_1D_ARRAY:
		case gli::TARGET_2D:
		case gli::TARGET_CUBE:
			if(gli::is_compressed(Texture.format()))
				glCompressedTexSubImage2D(
					_Target, static_cast<GLint>(Level),
					0, 0,
					Extent.x,
					Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
					Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
					Texture.data(Layer, Face, Level));
			else
				glTexSubImage2D(
					_Target, static_cast<GLint>(Level),
					0, 0,
					Extent.x,
					Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
					Format.External, Format.Type,
					Texture.data(Layer, Face, Level));
			break;
		case gli::TARGET_2D_ARRAY:
		case gli::TARGET_3D:
		case gli::TARGET_CUBE_ARRAY:
			if(gli::is_compressed(Texture.format()))
				glCompressedTexSubImage3D(
					_Target, static_cast<GLint>(Level),
					0, 0, Texture.target() == gli::TARGET_3D ? 0 : LayerGL,
					Extent.x, Extent.y,
					Texture.target() == gli::TARGET_3D ? Extent.z : 1,
					Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
					Texture.data(Layer, Face, Level));
			else
				glTexSubImage3D(
					_Target, static_cast<GLint>(Level),
                    0, 0, Texture.target() == gli::TARGET_3D ? 0 : (GLint)(LayerGL * Texture.faces() + Face),
					Extent.x, Extent.y,
                    Texture.target() == gli::TARGET_3D ? Extent.z : 1,
					Format.External, Format.Type,
					Texture.data(Layer, Face, Level));
			break;
		default: 
			assert(0); 
			break;
		}
	}
	m_Target = Target;
	m_TextureID = TextureID;
	m_Format = Format.Type;

    m_TextureDesc.setTarget(Texture.target());
    m_TextureDesc.setFormat(Texture.format());
    m_TextureDesc.setWidth(Extent.x);
    m_TextureDesc.setHeight(Extent.y);
	m_TextureDesc.setDepth(Texture.target() == gli::TARGET_3D ? Extent.z : FaceTotal);
    m_TextureDesc.setLevels((GLint)Texture.levels());

	return true;
}

bool OGLTexture::createFromMemoryHDR(const char* data, size_t size) noexcept
{
    stbi_set_flip_vertically_on_load(true);

	GLenum target = GL_TEXTURE_2D;
    GLenum type = GL_FLOAT;
    int width = 0, height = 0, nrComponents = 0;
    float* imagedata = stbi_loadf_from_memory((const stbi_uc*)data, (int)size, &width, &height, &nrComponents, 0);
    if (!imagedata) return false;

    GLenum Format = OGLTypes::getComponent(nrComponents);
    GLenum InternalFormat = OGLTypes::getInternalComponent(nrComponents, type == GL_FLOAT);

	gli::gl GL(gli::gl::PROFILE_GL33);
    gli::format format = GL.find(
        static_cast<gli::gl::internal_format>(InternalFormat),
        static_cast<gli::gl::external_format>(Format),
        static_cast<gli::gl::type_format>(type));

    bool bSuccess = create(width, height, target, format, 1, (const uint8_t*)imagedata, size);
    stbi_image_free(imagedata);
    return bSuccess;
}

bool OGLTexture::map(uint32_t mipLevel, std::uint8_t** data) noexcept
{
    using namespace gli;

    const GLsizei w = m_TextureDesc.getWidth(), h = m_TextureDesc.getHeight();

	assert(data);
    assert(m_TextureID != GL_NONE);

    const gl GL(gl::PROFILE_GL33);
    const swizzles swizzle(gl::SWIZZLE_RED, gl::SWIZZLE_GREEN, gl::SWIZZLE_BLUE, gl::SWIZZLE_ALPHA);
    const auto Format = GL.translate(m_TextureDesc.getFormat(), swizzle);

	GLsizei numBytes = OGLTypes::getFormatNumbytes(Format.External, Format.Type);
	if (numBytes == 0)
		return false;

	if (m_PBO == GL_NONE)
		glGenBuffers(1, &m_PBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO);

	GLsizei mapSize = w * h * numBytes;
	if (m_PBOSize < mapSize)
	{
		glBufferData(GL_PIXEL_PACK_BUFFER, mapSize, nullptr, GL_STREAM_READ);
		m_PBOSize = mapSize;
	}
	glBindTexture(m_Target, m_TextureID);
	glGetTexImage(m_Target, mipLevel, Format.External, Format.Type, nullptr);

	*data = (std::uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, mapSize, GL_MAP_READ_BIT);

	return *data ? true : false;
}

bool OGLTexture::map(uint32_t x, uint32_t y, uint32_t z, uint32_t w, uint32_t h, uint32_t d, std ::uint32_t mipLevel, std::uint8_t** data) noexcept
{
    assert(false);
    return false;
}

void OGLTexture::unmap() noexcept
{
	assert(m_PBO != GL_NONE);
	glUnmapNamedBuffer(m_PBO);
}
