#pragma once

#include <Types.h>

class BaseMesh
{
public:
	BaseMesh()
	{
	}

	virtual ~BaseMesh()
	{
		destroy();
	}

	void initialize();
	void destroy();
	void render();

	int32_t m_MaterialIndex = -1;
	unsigned int m_IndexBase = 0;
	unsigned int m_IndexCount = 0;
	unsigned int m_VertexBase = 0;
	BaseMaterialPtr m_Material;
};
