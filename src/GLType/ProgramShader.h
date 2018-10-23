#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <Math/Common.h>
#include <string>
#include <GraphicsTypes.h>
#include <vector>
#include <map>
#include <string_view>

class ProgramShader
{
public:
    ProgramShader() noexcept;
    virtual ~ProgramShader() noexcept;
    
    /** Generate the program id */
    bool initialize() noexcept;
    
    /** Destroy the program id */
    void destroy();        
    
    /** Add a shader and compile it */
    void addShader(GLenum shaderType, const std::string &tag);
    
    //bool compile(); //static (with param)?
    
    bool link(); //static (with param)?
    
    void bind() const { glUseProgram( m_ShaderID ); }
    void unbind() const { glUseProgram( 0u ); }
    
    /** Return the program id */
    GLuint getShaderID() const { return m_ShaderID; }
    
    bool initBlockBinding(const std::string& name);

    void setDevice(const GraphicsDevicePtr& device);

    bool setUniform(const std::string& name, GLint v) const;
    bool setUniform(const std::string& name, GLfloat v) const;
    bool setUniform(const std::string& name, const glm::vec2& v) const;
    bool setUniform(const std::string& name, const glm::vec3& v) const;
    bool setUniform(const std::string& name, const glm::vec4& v) const;
    bool setUniform(const std::string& name, const glm::vec2* v, size_t count) const;
    bool setUniform(const std::string& name, const glm::vec4* v, size_t count) const;
    bool setUniform(const std::string& name, const glm::mat3& v) const;
    bool setUniform(const std::string& name, const glm::mat4& v) const;
    bool bindTexture(const std::string& name, const GraphicsTexturePtr& texture, GLint unit);
    bool bindBuffer(const std::string& name, const GraphicsDataPtr& data);

    bool setUniform(GLint location, GLint v) const;
    bool setUniform(GLint location, GLfloat v) const;
    bool setUniform(GLint location, const glm::vec2& v) const;
    bool setUniform(GLint location, const glm::vec3& v) const;
    bool setUniform(GLint location, const glm::vec4& v) const;
    bool setUniform(GLint location, const glm::vec2* v, size_t count) const;
    bool setUniform(GLint location, const glm::vec4* v, size_t count) const;
    bool setUniform(GLint location, const glm::mat3& v) const;
    bool setUniform(GLint location, const glm::mat4& v) const;
    bool bindTexture(GLint location, const GraphicsTexturePtr& texture, GLint unit);

    // Compute
    bool bindImage(const std::string& name, const GraphicsTexturePtr& texture, GLint unit, GLint level, GLboolean layered, GLint layer, GLenum access);
    bool bindImage(const std::string& name, const OGLCoreTexturePtr& texture, GLint unit, GLint level, GLboolean layered, GLint layer, GLenum access);

    void Dispatch( GLuint GroupCountX = 1, GLuint GroupCountY = 1, GLuint GroupCountZ = 1 );
    void Dispatch1D( GLuint ThreadCountX, GLuint GroupSizeX = 64);
    void Dispatch2D( GLuint ThreadCountX, GLuint ThreadCountY, GLuint GroupSizeX = 8, GLuint GroupSizeY = 8);
    void Dispatch3D( GLuint ThreadCountX, GLuint ThreadCountY, GLuint ThreadCountZ, GLuint GroupSizeX = 4, GLuint GroupSizeY = 4, GLuint GroupSizeZ = 4 );
  
    static bool setIncludeFromFile(const std::string &includeName, const std::string &filename);
    static std::vector<char> readTextFile(const std::string &filename);

protected:

    void buildShader(GLenum shaderType, const std::string& tag, const std::string& content);

    static std::vector<std::string> directory;

    GLuint m_ShaderID;
    GLuint m_BlockPointCounter;
    GraphicsDeviceType m_DeviceType;
    GraphicsDeviceWeakPtr m_Device;
    std::map<std::string, GLuint> m_BlockPoints;
};

inline void ProgramShader::Dispatch( GLuint GroupCountX, GLuint GroupCountY, GLuint GroupCountZ )
{
    glDispatchCompute(GroupCountX, GroupCountY, GroupCountZ);
}

inline void ProgramShader::Dispatch1D( GLuint ThreadCountX, GLuint GroupSizeX )
{
    Dispatch( Math::DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1 );
}

inline void ProgramShader::Dispatch2D( GLuint ThreadCountX, GLuint ThreadCountY, GLuint GroupSizeX, GLuint GroupSizeY )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY), 1);
}

inline void ProgramShader::Dispatch3D( GLuint ThreadCountX, GLuint ThreadCountY, GLuint ThreadCountZ, GLuint GroupSizeX, GLuint GroupSizeY, GLuint GroupSizeZ )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY),
        Math::DivideByMultiple(ThreadCountZ, GroupSizeZ));
}
