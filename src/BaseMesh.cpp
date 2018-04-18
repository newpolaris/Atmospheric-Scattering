#include "BaseMesh.h"
#include <tools/gltools.hpp>

void BaseMesh::initialize()
{
}

void BaseMesh::destroy()
{
	m_Material = nullptr;
}

void BaseMesh::render()
{
	if (m_Material)
	{
	}

	GL_ASSERT(glDrawElementsBaseVertex(
				GL_TRIANGLES,
				m_IndexCount, 
				GL_UNSIGNED_INT,
				(void*)(sizeof(unsigned int)*m_IndexBase), 
				m_VertexBase));

	CHECKGLERROR();
}


