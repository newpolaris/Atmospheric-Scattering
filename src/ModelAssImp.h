#include <Types.h>
#include <GL/glew.h>

enum VertexAttrib
{
	VATTR_POSITION = 0,
	VATTR_NORMAL,
	VATTR_TEXCOORD,

	VATTR_COUNT
};

class ModelAssImp
{
public:
	ModelAssImp();
	virtual ~ModelAssImp();

	void create();
	void destroy();
	void render();

	bool loadFromFile(const std::string& filename);

	GLuint m_VAO;
	GLuint m_IBO;
	GLuint m_VBO[VATTR_COUNT];

	BaseMeshList m_Meshes;
	BaseMaterialList m_Materials;
};

