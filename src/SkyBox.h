/**
 * 
 *    \file SkyBox.hpp
 * 
 * 
 */
 
#pragma once

#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include <vector>
#include <string>
#include <memory>

class TCamera;
class ProgramShader;
class CubeMesh;

class SkyBox
{
  protected:
    bool m_bInitialized;
    
    ProgramShader *m_Program;
    CubeMesh *m_CubeMesh;
    
    std::vector<std::shared_ptr<class OGLCoreTexture>> m_cubemaps;
    size_t m_curIdx;
    
    //-------------------------------------------------
    bool m_bAutoRotation;
    float m_spin;
    glm::mat4 m_rotateMatrix;
    glm::mat3 m_invRotateMatrix; //it's for normals, so we don't need a 4x4 matrix
    //-------------------------------------------------
    
    
  public:
    SkyBox()
      : m_bInitialized(false),
        m_Program(0),
        m_CubeMesh(0),
        m_curIdx(0u),
        m_rotateMatrix(1.f),
        m_invRotateMatrix(1.f),
        m_bAutoRotation(false),
        m_spin(0.0f)
    {}
    
    virtual ~SkyBox();
    
    void initialize();
	void shutdown();
    void render(const TCamera& camera);
    
    void addCubemap( const std::string &name );
    bool setCubemap( size_t idx );
    
    //-------------------------------------------------
    void toggleAutoRotate() {m_bAutoRotation = !m_bAutoRotation;}
    const glm::mat3& getInvRotateMatrix() {return m_invRotateMatrix;}
    //-------------------------------------------------
};

#endif //SKYBOX_HPP
