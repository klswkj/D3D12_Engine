#include "stdafx.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "ModelComponentWindow.h"
#include "MasterRenderGraph.h"
#include "ObjectFilterFlag.h"

inline std::string _NormalizeString(const std::string& InputString)
{
	size_t FirstDotIndex = -1;       // std::string::npos
	size_t FirstBackSlashIndex = -1; // std::string::npos 

	FirstDotIndex = InputString.rfind('.');        // strrchr(InputString.c_str(), '.');
	FirstBackSlashIndex = InputString.rfind('\\'); // strrchr(InputString.c_str(), '\\');

	ASSERT(FirstDotIndex != std::string::npos && FirstBackSlashIndex != std::string::npos);

	return InputString.substr(FirstBackSlashIndex + 1, FirstDotIndex - FirstBackSlashIndex - 1);
}

inline std::string SplitFormat(const std::string InputString)
{
	size_t FirstDotIndex = -1;

	FirstDotIndex = InputString.rfind('.'); // // strrchr(InputString.c_str(), '.');

	ASSERT(FirstDotIndex != std::string::npos);

	return InputString.substr(0, FirstDotIndex);
}

Model::Model(const std::string& pathString, float scale/* = 1.0f*/)
{
	Assimp::Importer AssImporter;
	AssImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
		aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS);
	// max triangles and vertices per mesh, splits above this threshold
	AssImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, INT_MAX);
	AssImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 0xfffe); // avoid the primitive restart index
	// remove points and lines
	AssImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

	const aiScene* pScene = AssImporter.ReadFile(pathString,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_OptimizeMeshes
	);

	ASSERT(pScene != nullptr);

	m_ModelName = _NormalizeString(pathString);

	std::vector<Material> materials;

	materials.reserve(pScene->mNumMaterials);

	for (size_t i = 0; i < pScene->mNumMaterials; ++i)
	{
		materials.emplace_back(*pScene->mMaterials[i], pathString);
	}

	// 모두 통합하는 코드.
	// void Entity::LoadMesh(const aiMesh & _aiMesh, VertexLayout * *pVertexLayout, uint16_t * *pIndices)

	for (size_t i = 0; i < pScene->mNumMeshes; ++i)
	{
		const aiMesh& mesh = *pScene->mMeshes[i];
		m_pMeshes.push_back(std::make_unique<Mesh>(materials[mesh.mMaterialIndex], mesh, scale));
	}

	uint32_t nextId = 0;
	m_pModelPart = ParseNode(nextId, *pScene->mRootNode, scale);
}

void Model::Submit(eObjectFilterFlag Filter) const DEBUG_EXCEPT
{
	m_pModelPart->Submit(Filter, DirectX::XMMatrixIdentity());
}

void Model::SetRootTransform(DirectX::XMMATRIX _Transform) noexcept
{
	m_pModelPart->SetAppliedTransform(_Transform);
}

void Model::Accept(ModelComponentWindow& _ComponentWindow)
{
	m_pModelPart->RecursivePushComponent(_ComponentWindow);
}

void Model::LinkTechniques(MasterRenderGraph& _MasterRenderGraph)
{
	for (auto& pMesh : m_pMeshes)
	{
		pMesh->LinkTechniques(_MasterRenderGraph);
	}
}

std::unique_ptr<ModelPart> Model::ParseNode(uint32_t& NextId, const aiNode& _aiNode, float _Scale) noexcept
{
	auto ScaleTranslation = [&](DirectX::XMMATRIX _Matrix, float scale)
	{
		_Matrix.r[3].m128_f32[0] *= scale;
		_Matrix.r[3].m128_f32[1] *= scale;
		_Matrix.r[3].m128_f32[2] *= scale;
		return _Matrix;
	};

	const DirectX::XMMATRIX TransformMatrix = ScaleTranslation
	(
		DirectX::XMMatrixTranspose
		(
			DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&_aiNode.mTransformation))
		), _Scale
	);

	std::vector<Mesh*> pCurrentMeshes;
	pCurrentMeshes.reserve(_aiNode.mNumMeshes);

	for (size_t IMesh = 0; IMesh < _aiNode.mNumMeshes; ++IMesh)
	{
		const size_t CurrentMesh = _aiNode.mMeshes[IMesh];
		pCurrentMeshes.push_back(m_pMeshes[CurrentMesh].get());
	}

	// std::string _NormalizedString = SplitFormat(std::string(_aiNode.mName.C_Str()));

	auto pNode = std::make_unique<ModelPart>(NextId++, _aiNode.mName.C_Str()/*_NormalizedString*/, std::move(pCurrentMeshes), TransformMatrix);

	for (size_t IChild = 0; IChild < _aiNode.mNumChildren; ++IChild)
	{
		// pNode->AddChild(ParseNode(NextId, *_aiNode.mChildren[IChild], _NormalizedString, _Scale));
		pNode->AddChild(ParseNode(NextId, *_aiNode.mChildren[IChild], _Scale));
	}

	return pNode;
}
/*
std::unique_ptr<ModelPart> Model::ParseNode(uint32_t& NextId, const aiNode& _aiNode, std::string NormalizedString, float _Scale)
{
	auto ScaleTranslation = [&](DirectX::XMMATRIX _Matrix, float scale)
	{
		_Matrix.r[3].m128_f32[0] *= scale;
		_Matrix.r[3].m128_f32[1] *= scale;
		_Matrix.r[3].m128_f32[2] *= scale;
		return _Matrix;
	};

	const DirectX::XMMATRIX TransformMatrix = ScaleTranslation
	(
		DirectX::XMMatrixTranspose
		(
			DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&_aiNode.mTransformation))
		), _Scale
	);

	std::vector<Mesh*> pCurrentMeshes;
	pCurrentMeshes.reserve(_aiNode.mNumMeshes);

	for (size_t IMesh = 0; IMesh < _aiNode.mNumMeshes; ++IMesh)
	{
		const size_t CurrentMesh = _aiNode.mMeshes[IMesh];
		pCurrentMeshes.push_back(m_pMeshes[CurrentMesh].get());
	}

	auto pNode = std::make_unique<ModelPart>(NextId++, _NormalizeString(std::string(_aiNode.mName.C_Str())), std::move(pCurrentMeshes), TransformMatrix);

	for (size_t IChild = 0; IChild < _aiNode.mNumChildren; ++IChild)
	{
		pNode->AddChild(ParseNode(NextId, *_aiNode.mChildren[IChild], _Scale));
	}

	return pNode;
}
*/