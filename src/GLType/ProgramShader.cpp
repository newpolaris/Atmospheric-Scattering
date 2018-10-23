#include <cstdio>
#include <cassert>

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glsw/glsw.h>

#include <tools/gltools.hpp>
#include <tools/Logger.hpp>
#include <tools/FileUtility.h>
#include <GLType/ProgramManager.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLCoreGraphicsData.h>
#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>

#include "ProgramShader.h"

static std::vector<std::string> directory = { ".", "./shaders" };

ProgramShader::ProgramShader() noexcept
    : m_ShaderID(0u)
    , m_BlockPointCounter(0u)
{
}

ProgramShader::~ProgramShader() noexcept
{
    destroy(); 
}

bool ProgramShader::initialize() noexcept
{
    assert(m_ShaderID == GL_NONE);
    if (m_ShaderID != GL_NONE)
        return false;

    m_ShaderID = glCreateProgram();

#ifdef GL_ARB_separate_shader_objects
    glProgramParameteri(m_ShaderID, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_FALSE);
    glProgramParameteri(m_ShaderID, GL_PROGRAM_SEPARABLE, GL_FALSE);
#endif
    return true;
}

void ProgramShader::destroy()
{
    if (m_ShaderID) {
        glDeleteProgram(m_ShaderID);
        m_ShaderID = 0;
    }
}

void ProgramShader::addShader(GLenum shaderType, const std::string &tag)
{
    // require initialization
    assert(m_ShaderID > 0);

    if (glswGetError() != 0) {
        fprintf(stderr, "GLSW : %s", glswGetError());
    }
    assert(glswGetError() == 0);

    util::BytesArray source = util::ReadFileSync("./shaders/" + tag);
    if (source != util::NullFile)
    {
        std::string content((const char*)source->data());
        buildShader(shaderType, tag, content);
        return;
    }
    else
    {
        const GLchar *source = glswGetShader(tag.c_str());
        if (0 != source)
        {
            std::string content(source);
            buildShader(shaderType, tag, content);
            return;
        }
    }

    fprintf(stderr, "Error : shader \"%s\" not found, check your directory.\n", tag.c_str());
    fprintf(stderr, "Execution terminated.\n");
    exit(EXIT_FAILURE);
}

void ProgramShader::buildShader(GLenum shaderType, const std::string& tag, const std::string& content)
{
    static nv_helpers_gl::IncludeRegistry m_includes;
    static std::vector<std::string> directory = { ".", "./shaders" };
    const std::string preprocessed = nv_helpers_gl::manualInclude(tag, content, "", directory, m_includes);
    char const* sourcePointer = preprocessed.c_str();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &sourcePointer, 0);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE)
    {
        //Logger::getInstance().write( "shader \"%s\" compilation failed.\n", cTag);
        fprintf(stderr, "%s compilation failed.\n", tag.c_str());
        gltools::printShaderLog(shader);
        exit(EXIT_FAILURE);
    }

    //Logger::getInstance().write( "%s compiled.\n", cTag);
    fprintf(stderr, "%s compiled.\n", tag.c_str());

    glAttachShader(m_ShaderID, shader);
    glDeleteShader(shader);     //flag for deletion
}

bool ProgramShader::link()
{
    glLinkProgram(m_ShaderID);

    // Test linking
    GLint status = 0;
    glGetProgramiv(m_ShaderID, GL_LINK_STATUS, &status);

    if(status != GL_TRUE)
    {
        fprintf(stderr, "program linking failed.\n");
        return false;
    }

    return true;
}

bool ProgramShader::initBlockBinding(const std::string& name)
{
    GLint block = glGetUniformBlockIndex(m_ShaderID, name.c_str());
    if (-1 == block)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniformBlockBinding(m_ShaderID, block, m_BlockPointCounter);
    m_BlockPoints.insert({name, m_BlockPointCounter});
    m_BlockPointCounter++;
    return true;
}

void ProgramShader::setDevice(const GraphicsDevicePtr& device)
{
    assert(device);
    m_Device = device;
    m_DeviceType = device->getGraphicsDeviceDesc().getDeviceType();
}

bool ProgramShader::setUniform(const std::string &name, GLint v) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if(-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniform1i(loc, v);
    return true;
}


bool ProgramShader::setUniform(const std::string &name, GLfloat v) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if(-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniform1f(loc, v);
    return true;
}

bool ProgramShader::setUniform(const std::string& name, const glm::vec2& v) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if(-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniform2fv(loc, 1, glm::value_ptr(v));
    return true;
}

bool ProgramShader::setUniform(const std::string &name, const glm::vec3 &v) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if(-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniform3fv(loc, 1, glm::value_ptr(v));
    return true;
}

bool ProgramShader::setUniform(const std::string &name, const glm::vec4 &v) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if(-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniform4fv(loc, 1, glm::value_ptr(v));
    return true;
}

bool ProgramShader::setUniform(const std::string& name, const glm::vec2* v, size_t count) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());
    if (-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    if (count == 0)
        return true;

    glUniform2fv(loc, count, glm::value_ptr(*v));
    return true;
}

bool ProgramShader::setUniform(const std::string& name, const glm::vec4* v, size_t count) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());
    if (-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    if (count == 0)
        return true;

    glUniform4fv(loc, count, glm::value_ptr(*v));
    return true;
}

bool ProgramShader::setUniform(const std::string& name, const glm::mat3& v) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if(-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(v));
    return true;
}

bool ProgramShader::setUniform(const std::string &name, const glm::mat4 &v) const
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if(-1 == loc)
    {
        printf("ProgramShader : can't find uniform \"%s\".\n", name.c_str());
        return false;
    }

    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(v));
    return true;
}

bool ProgramShader::bindTexture(const std::string& name, const GraphicsTexturePtr& texture, GLint unit)
{
    assert(texture);
    assert(unit >= 0);
    assert(m_ShaderID != 0);

    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if (-1 == loc)
    {
        printf("ProgramShader : can't find texture \"%s\".\n", name.c_str());
        return false;
    }

    // Bind the buffer object to the texture 
    if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto tex = texture->downcast_pointer<OGLCoreTexture>();
        tex->bind(unit);
        glUniform1i(loc, unit);
        return true;
    }
    else if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto tex = texture->downcast_pointer<OGLTexture>();
        tex->bind(unit);
        glUniform1i(loc, unit);
        return true;
    }
    return false;
}

bool ProgramShader::bindBuffer(const std::string& name, const GraphicsDataPtr& data)
{
    auto device = m_Device.lock();
    if (!device) return false;
    auto it = m_BlockPoints.find(name);
    if (it == m_BlockPoints.end())
        return false;

    auto blockPoint = it->second;

    // Bind the buffer object to the uniform block
    if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto ubo = data->downcast_pointer<OGLCoreGraphicsData>();
        glBindBufferBase(GL_UNIFORM_BUFFER, blockPoint, ubo->getInstanceID());
        return true;
    }
    else if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto ubo = data->downcast_pointer<OGLGraphicsData>();
        glBindBufferBase(GL_UNIFORM_BUFFER, blockPoint, ubo->getInstanceID());
        return true;
    }
    return false;
}

bool ProgramShader::setUniform(GLint location, GLint v) const
{
    if (location < 0) return false;
    glUniform1i(location, v);
    return true;
}

bool ProgramShader::setUniform(GLint location, GLfloat v) const
{
    if (location < 0) return false;
    glUniform1f(location, v);
    return true;
}

bool ProgramShader::setUniform(GLint location, const glm::vec3& v) const
{
    if (location < 0) return false;
    glUniform3fv(location, 1, glm::value_ptr(v));
    return true;
}

bool ProgramShader::setUniform(GLint location, const glm::mat4& v) const
{
    if (location < 0) return false;
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(v));
    return true;
}

bool ProgramShader::bindTexture(GLint location, const GraphicsTexturePtr& texture, GLint unit)
{
    if (location < 0) return false;
    // Bind the buffer object to the texture 
    if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto tex = texture->downcast_pointer<OGLCoreTexture>();
        tex->bind(unit);
        glUniform1i(location, unit);
        return true;
    }
    else if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto tex = texture->downcast_pointer<OGLTexture>();
        tex->bind(unit);
        glUniform1i(location, unit);
        return true;
    }
    return true;
}

bool ProgramShader::bindImage(const std::string& name, const GraphicsTexturePtr& texture, GLint unit, GLint level, GLboolean layered, GLint layer, GLenum access)
{
    if (!texture) return false;
    auto tex = texture->downcast_pointer<OGLCoreTexture>();
    if (!tex) return false;
    return bindImage(name, tex, unit, level, layered, layer, access);
}

bool ProgramShader::bindImage(const std::string& name, const OGLCoreTexturePtr &texture,
    GLint unit, GLint level, GLboolean layered, GLint layer, GLenum access)
{
    GLint loc = glGetUniformLocation(m_ShaderID, name.c_str());

    if (-1 == loc)
    {
        printf("ProgramShader : can't find image \"%s\".\n", name.c_str());
        return false;
    }

    // Bind the buffer object to the uniform block
    if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto tex = texture->downcast_pointer<OGLCoreTexture>();
        glBindImageTexture(unit, tex->getTextureID(), level, layered, layer, access, tex->getFormat());
        glUniform1i(loc, unit);
        return true;
    }
    else if (m_DeviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto tex = texture->downcast_pointer<OGLTexture>();
        glBindImageTexture(unit, tex->getTextureID(), level, layered, layer, access, tex->getFormat());
        glUniform1i(loc, unit);
        return true;
    }

    return true;
}

bool ProgramShader::setIncludeFromFile(const std::string &includeName, const std::string &filename)
{
    auto incStr = readTextFile(filename);
    if(incStr.size() == 0)
        return false;
    glNamedStringARB(GL_SHADER_INCLUDE_ARB, includeName.size(), includeName.c_str(), incStr.size(), incStr.data());
    return false;
}

std::vector<char> ProgramShader::readTextFile(const std::string &filename)
{
    if (filename.empty()) 
        return std::vector<char>();

	FILE *fp = 0;
	if (!(fp = fopen(filename.c_str(), "r")))
	{
		printf("Cannot open \"%s\" for read!\n", filename.c_str());
		return std::vector<char>();
	}

    fseek(fp, 0L, SEEK_END);     // seek to end of file
    long size = ftell(fp);       // get file length
    rewind(fp);                  // rewind to start of file

    std::vector<char> buffer(size);

	size_t bytes;
	bytes = fread(buffer.data(), 1, size, fp);

	fclose(fp);
	return buffer;
}
