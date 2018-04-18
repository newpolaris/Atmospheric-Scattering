/**
 * 
 *    \file Mesh.h
 * 
 *     \todo : Placer la geometrie GPU (VAO, VBO,..) dans une structure et
 *             liée Matrice + BufferObject dans un même ensemble.
 *             Donc simplifier le concept de 'Mesh' à l'objet seul, sans 
 *             transformation.
 * 
 */


#pragma once

#ifndef MESH_HPP
#define MESH_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <GLType/VertexBuffer.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923
#endif




/** MESH ---------------------------------------------- */

class Mesh
{
protected:
	bool m_bInitialized;    

	VertexBuffer m_vertexBuffer;
	GLsizei m_count;

	/* TODO Move in another object */
	glm::mat4 m_model;
	glm::mat3 m_normal; // 'kind of' the model matrix (different for the scale)
	// each time the model matrix is change, it has to change too !
	// (for lighted objects)


public:
	Mesh()
		: m_bInitialized(false), m_model(1.f), m_normal(1.f)
	{}

	virtual ~Mesh() { destroy(); }

	virtual void create() {}
	virtual void draw() const {}
	virtual void destroy();

	void setModelMatrix(const glm::mat4 &model)     {m_model = model;}
	void setNormalMatrix(const glm::mat3 &normal)   {m_normal = normal;}

	const glm::mat4& getModelMatrix() const   {return m_model;}
	const glm::mat3& getNormalMatrix() const  {return m_normal;}
};


/** PLANE MESH ----------------------------------------- */

class PlaneMesh : public Mesh
{
protected:
	float m_size;
	float m_res;
	float m_UVScale;    

public:
	PlaneMesh(float size = 100.0f, float res = 32.f, float uvscale = 1.f)
		: Mesh()
		  , m_size(size)
		  , m_res(res)
		  , m_UVScale(uvscale)
	{
	}

	void create() override;
	void draw() const override;
};


/** SPHERE MESH ---------------------------------------- */

class SphereMesh : public Mesh
{
protected:
	int m_meshResolution;
	float m_radius;

public:
	SphereMesh(int res = 16, float radius = 1.0f)
		: Mesh(),
		m_meshResolution(res),
		m_radius(radius)
	{}

	void create() override;
	void draw() const override;
};


/** CONE MESH ------------------------------------------ */

class ConeMesh : public Mesh
{
public:
	ConeMesh()
		: Mesh()
	{}

	void create() override;
	void draw() const override;
};


/** CUBE MESH ------------------------------------------ */

class CubeMesh : public Mesh
{
public:
	CubeMesh()
		: Mesh()
	{}

	void create() override;
	void draw() const override;
};

/** FULL SCREEN TRIANGLE MESH -------------------------- */

class FullscreenTriangleMesh : public Mesh
{
public:
	FullscreenTriangleMesh()
		: Mesh()
	{}

	void create() override;
	void draw() const override;
};

#endif //MESH_HPP
