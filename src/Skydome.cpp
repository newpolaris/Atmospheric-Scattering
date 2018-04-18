/**
 * 
 *    \file Skydome.cpp
 * 
 * 
 */

#include <cassert>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <tools/TCamera.h>
#include <tools/gltools.hpp>
#include <tools/Timer.hpp>
#include <GLType/ProgramShader.h>
#include <GLType/OGLCoreTexture.h>
#include "Mesh.h"
#include "Skydome.h"

Skydome::~Skydome()
{
	shutdown();
}

void Skydome::shutdown()
{
	if (0 != m_Program) delete m_Program;
	if (0 != m_SphereMesh) delete m_SphereMesh;
	m_Program = 0;
	m_SphereMesh = 0;
	m_texture = nullptr;
}

void Skydome::initialize()
{
	assert( false == m_bInitialized );

	// Create & load the CubeMap program
	m_Program = new ProgramShader();  
	m_Program->initialize();
	m_Program->addShader( GL_VERTEX_SHADER, "Skydome.Vertex" );
	m_Program->addShader( GL_FRAGMENT_SHADER, "Skydome.Fragment" );
	m_Program->link();

	// Create the cube mesh
	m_SphereMesh = new SphereMesh(32, 10);
	m_SphereMesh->create();

	m_bInitialized = true;
}

void Skydome::render(const TCamera& camera)
{
	assert( m_bInitialized );  

	if (m_texture == nullptr)
	{
		fprintf(stderr, "ERROR : no texture specified\n");
		exit(0);
	}

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );  
	glDisable( GL_CULL_FACE );  

	m_Program->bind();
	{    
		//-------------------------------------------------
		if (m_bAutoRotation)
		{
			m_spin = fmodf( m_spin+0.1f, 360.0f);
			glm::vec3 axis = glm::vec3( 1.0f, 0.7f, -0.5f );

			m_rotateMatrix = glm::rotate( glm::mat4(1.0f), m_spin, axis);
			m_invRotateMatrix = glm::mat3( glm::rotate( glm::mat4(1.0f), -m_spin, axis) );
		}
		//-------------------------------------------------

		// Vertex uniform
		glm::mat4 scale = glm::scale(glm::vec3(1.f, -1.f, 1.f));
		glm::mat4 followCamera = glm::translate( glm::mat4(1.0f), camera.getPosition());
		glm::mat4 model = m_SphereMesh->getModelMatrix() * followCamera * m_rotateMatrix;
		glm::mat4 mvp = camera.getViewProjMatrix() * model * scale;    
		m_Program->setUniform( "uModelViewProjMatrix", mvp);

		// Fragment uniform
		m_Program->setUniform( "uCubemap", 0);

		m_texture->bind( 0u );
		m_SphereMesh->draw();    
		m_texture->unbind( 0u );
	}
	m_Program->unbind();

	//glEnable( GL_CULL_FACE );
	glDepthMask( GL_TRUE );
	glEnable( GL_DEPTH_TEST ); 

	CHECKGLERROR();
}

void Skydome::setTexture( const std::string &name )
{
	assert( m_bInitialized );

	m_texture = std::make_shared<OGLCoreTexture>();
	m_texture->create( name );  
}

void Skydome::setTexture( std::shared_ptr<OGLCoreTexture>& texture )
{
	m_texture = texture;
}
