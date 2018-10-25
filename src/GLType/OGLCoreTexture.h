#pragma once

#include <GL/glew.h>
#include <string>
#include <GraphicsTypes.h>
#include <tools/Rtti.h>
#include <GLType/GraphicsTexture.h>

class OGLCoreTexture final : public GraphicsTexture
{
	__DeclareSubInterface(OGLCoreTexture, GraphicsTexture)
public:

	OGLCoreTexture();
    virtual ~OGLCoreTexture();

    bool create(const GraphicsTextureDesc& desc) noexcept;
	bool create(const std::string& filename) noexcept;
	bool create(GLint width, GLint height, GLenum target, GraphicsFormat format, GLuint levels, const uint8_t* data, uint32_t size) noexcept;
    bool create(GLint width, GLint height, GLint depth, GLenum target, GraphicsFormat format, GLuint levels, const uint8_t* data, uint32_t size) noexcept;
	void destroy() noexcept;
	void bind(GLuint unit) const;
	void unbind(GLuint unit) const;
	void generateMipmap();

    virtual bool map(std::uint32_t mipLevel, std::uint8_t** data) noexcept override;
    virtual bool map(std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint32_t w, std::uint32_t h, std::uint32_t d, std::uint32_t mipLevel, std::uint8_t** data) noexcept override; 
	void unmap() noexcept override;

    GLuint getTextureID() const noexcept;
    GLenum getFormat() const noexcept;
    GLenum getType() const noexcept;

    const GraphicsTextureDesc& getGraphicsTextureDesc() const noexcept override;

    // temp
	void parameteri(GLenum pname, GLint param);
	void parameterf(GLenum pname, GLfloat param);

private:

    void applyParameters(const GraphicsTextureDesc& desc);

    bool createFromMemory(const char* data, size_t dataSize) noexcept;
    bool createFromMemoryDDS(const char* data, size_t dataSize) noexcept; // DDS, KTX
    bool createFromMemoryHDR(const char* data, size_t dataSize) noexcept; // HDR
    bool createFromMemoryLDR(const char* data, size_t dataSize) noexcept; // JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC files
    bool createFromMemoryZIP(const char* data, size_t dataSize) noexcept; // ZLIB

private:

	friend class OGLDevice;
	void setDevice(const GraphicsDevicePtr& device) noexcept;
	GraphicsDevicePtr getDevice() noexcept;

private:

	OGLCoreTexture(const OGLCoreTexture&) noexcept = delete;
	OGLCoreTexture& operator=(const OGLCoreTexture&) noexcept = delete;

private:

    GraphicsTextureDesc m_TextureDesc;

	GLuint m_TextureID;
	GLenum m_Target;
	GLenum m_Format;
    GLenum m_Type;
	GLuint m_PBO;
	GLsizei m_PBOSize;
	GraphicsDeviceWeakPtr m_Device;
};

