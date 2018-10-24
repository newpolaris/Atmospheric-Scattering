#include <ModelAssImp.h>
#include <tools/gltools.hpp>

// GLM for matrix transformation
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

// assimp
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing fla

#include "BaseMesh.h"

ModelAssImp::ModelAssImp()
{
	m_VAO = 0;
	m_IBO = 0;
	for (unsigned int i = 0; i < VATTR_COUNT; i++)
		m_VBO[i] = 0;
}

ModelAssImp::~ModelAssImp()
{
	destroy();
}

void ModelAssImp::create()
{
	glGenVertexArrays(1, &m_VAO);   
	glGenBuffers(VATTR_COUNT, m_VBO);   
	glGenBuffers(1, &m_IBO);   
}

void ModelAssImp::destroy()
{
	if (m_IBO)
	{
		glDeleteBuffers(1, &m_IBO);
		m_IBO = 0;
	}

	glDeleteBuffers(VATTR_COUNT, m_VBO);
	for (unsigned int i = 0; i < VATTR_COUNT; i++)
		m_VBO[i] = 0;

	if (m_VAO)
	{
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}

	m_Meshes.clear();
	m_Materials.clear();
}

bool ModelAssImp::loadFromFile(const std::string& filename)
{
	Assimp::Importer Importer;

#if _DEBUG
    int postprocess = aiProcess_Triangulate 
        | aiProcess_GenSmoothNormals 
        | aiProcess_FlipUVs 
        | aiProcess_JoinIdenticalVertices
        ;
#else
	int postprocess = aiProcess_Triangulate 
		| aiProcess_GenSmoothNormals 
		| aiProcess_SplitLargeMeshes
		| aiProcess_SortByPType
		| aiProcess_OptimizeMeshes
		| aiProcess_CalcTangentSpace
		| aiProcess_JoinIdenticalVertices
#endif

	const aiScene* pScene = Importer.ReadFile(filename.c_str(), postprocess);
	assert(pScene != nullptr);
	if (!pScene) return false;

	// Handle embeded textures
	if (pScene->HasTextures())
	{
	}

	unsigned int NumIndices = 0;
	unsigned int NumVertices = 0;
	auto NumMeshes = pScene->mNumMeshes;
	auto NumMaterials = pScene->mNumMaterials;

	for (auto i = 0u; i < NumMeshes; i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];
		NumVertices += paiMesh->mNumVertices;
		NumIndices += paiMesh->mNumFaces*3;
	}

	// material
	for (uint32_t i = 0; i < pScene->mNumMaterials; i++)
	{

	}

	// Apppend all vertex and index
	std::vector<glm::vec3> positions(NumVertices);
	std::vector<glm::vec3> normals(NumVertices);
	std::vector<glm::vec2> texcoords(NumVertices);
	std::vector<unsigned int> indices(NumIndices);

	unsigned int baseVert = 0u, baseIdx = 0u;
	for (uint32_t meshIdx = 0; meshIdx < NumMeshes; meshIdx++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[meshIdx];
		BaseMeshPtr mesh = std::make_shared<BaseMesh>();
		mesh->m_MaterialIndex = paiMesh->mMaterialIndex;
		if (mesh->m_MaterialIndex < m_Materials.size()) 
			mesh->m_Material = m_Materials[mesh->m_MaterialIndex];

		// vertex buffer
		bool bHasTex = paiMesh->HasTextureCoords(0);
		const aiVector3D zero(0.f, 0.f, 0.f);
		for (int i = 0u; i < NumVertices; i++)
		{
			const aiVector3D& pos = paiMesh->mVertices[i];
			const aiVector3D& nor = paiMesh->mNormals[i];
			const aiVector3D& tex = bHasTex ? paiMesh->mTextureCoords[0][i] : zero;

			positions[baseVert + i] = glm::vec3(pos.x, pos.y, pos.z);
			texcoords[baseVert + i] = glm::vec2(tex.x, tex.y);
			normals[baseVert + i] = glm::vec3(nor.x, nor.y, nor.z);
		}

		// index buffer 
		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
			const aiFace& face = paiMesh->mFaces[i];
            const unsigned int base = i*3;
			for (unsigned int j = 0; j < 3; j++)
				indices[baseIdx + base + j] = face.mIndices[j];
		}

		mesh->m_VertexBase = baseVert;
		mesh->m_IndexBase = baseIdx;
		mesh->m_IndexCount = paiMesh->mNumFaces*3;
		m_Meshes.push_back(mesh);

		baseVert += paiMesh->mNumVertices;
		baseIdx += paiMesh->mNumFaces*3;
	}

    GL_ASSERT(glBindVertexArray(m_VAO));

	GL_ASSERT(glBindBuffer(GL_ARRAY_BUFFER, m_VBO[VATTR_POSITION]));
	GL_ASSERT(glBufferData(GL_ARRAY_BUFFER, sizeof(positions[0])*positions.size(), positions.data(), GL_STATIC_DRAW));
	GL_ASSERT(glEnableVertexAttribArray(VATTR_POSITION));
	GL_ASSERT(glVertexAttribPointer(VATTR_POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0));

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO[VATTR_NORMAL]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals[0])*normals.size(), normals.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(VATTR_NORMAL);
	glVertexAttribPointer(VATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO[VATTR_TEXCOORD]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords[0])*texcoords.size(), texcoords.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(VATTR_TEXCOORD);
	glVertexAttribPointer(VATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0])*indices.size(), indices.data(), GL_STATIC_DRAW);
	
    glBindVertexArray(0);	

	CHECKGLERROR();

	return true;
}

void ModelAssImp::render()
{
    glBindVertexArray(m_VAO);	
	for (auto& mesh : m_Meshes)
		mesh->render();
    glBindVertexArray(0);	
}
