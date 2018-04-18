#pragma once
#include <Types.h>

class BaseMaterial
{
public:
	BaseMaterial()
	{
	}

	virtual ~BaseMaterial()
	{
		destroy();
	}

	void initialize();
	void destroy();
};

