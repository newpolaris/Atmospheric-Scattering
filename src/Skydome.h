/**
 * 
 *    \file Skydome.hpp
 * 
 * 
 */
 
#pragma once

#include <vector>
#include <string>
#include <memory>

class TCamera;
class ProgramShader;
class SphereMesh;
class Texture2D;
class OGLCoreTexture;

class Skydome
{
  protected:
    bool m_bInitialized;
    
    ProgramShader *m_Program;
    SphereMesh *m_SphereMesh;
	  std::shared_ptr<OGLCoreTexture> m_texture;
    
    //-------------------------------------------------
    bool m_bAutoRotation;
    float m_spin;
    glm::mat4 m_rotateMatrix;
    glm::mat3 m_invRotateMatrix; //it's for normals, so we don't need a 4x4 matrix
    //-------------------------------------------------
    
    
  public:
    Skydome()
      : m_bInitialized(false),
        m_Program(0),
        m_SphereMesh(0),
        m_bAutoRotation(false),
        m_spin(0.0f)
    {}
    
    virtual ~Skydome();
    
    void initialize();
	void shutdown();
    void render(const TCamera& camera);
    
    void setTexture( const std::string &name );
    void setTexture( std::shared_ptr<OGLCoreTexture>& texture );
    
    //-------------------------------------------------
    void toggleAutoRotate() {m_bAutoRotation = !m_bAutoRotation;}
    const glm::mat3& getInvRotateMatrix() {return m_invRotateMatrix;}
    //-------------------------------------------------
};
