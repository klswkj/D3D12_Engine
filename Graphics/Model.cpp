#include "stdafx.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Model.h"
#include "Mesh.h"
#include "ModelPart.h"
#include "ComponentWindow.h"
#include "MasterRenderGraph.h"

Model::Model(const char* pathString, float scale/* = 1.0f*/)
{
	Assimp::Importer imp;
	const auto pScene = imp.ReadFile(pathString,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace
	);

	ASSERT(pScene == nullptr);

	// parse materials
	std::vector<Material> materials;
	materials.reserve(pScene->mNumMaterials); // pScene->mNumMaterials <=> m_Header.materialCount
	for (size_t i = 0; i < pScene->mNumMaterials; ++i)
	{
		materials.emplace_back(*pScene->mMaterials[i], pathString);
	}

	for (size_t i = 0; i < pScene->mNumMeshes; ++i)
	{
		const aiMesh& mesh = *pScene->mMeshes[i];
		meshPtrs.push_back(std::make_unique<Mesh>(materials[mesh.mMaterialIndex], mesh, scale));
	}

	uint32_t nextId = 0;
	pRoot = ParseNode(nextId, *pScene->mRootNode, scale);
}

void Model::Submit(size_t channels) const DEBUG_EXCEPT
{
	pRoot->Submit(channels, DirectX::XMMatrixIdentity());
}

void Model::SetRootTransform(DirectX::FXMMATRIX tf) noexcept
{
	pRoot->SetAppliedTransform(tf);
}

void Model::Accept(ComponentWindow& probe)
{
	pRoot->RecursivePushComponent(probe);
}

void Model::LinkTechniques(MasterRenderGraph& rg)
{
	for (auto& pMesh : meshPtrs)
	{
		pMesh->LinkTechniques(rg);
	}
}

std::unique_ptr<ModelPart> Model::ParseNode(uint32_t& nextId, const aiNode& node, float scale) noexcept
{
	auto ScaleTranslation = [&](DirectX::XMMATRIX _Matrix, float scale)
	{
		_Matrix.r[3].m128_f32[0] *= scale;
		_Matrix.r[3].m128_f32[1] *= scale;
		_Matrix.r[3].m128_f32[2] *= scale;
		return _Matrix;
	};

	const auto transform = ScaleTranslation
	(
		DirectX::XMMatrixTranspose
		(
			DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&node.mTransformation))
		), 
		scale
	);

	std::vector<Mesh*> curMeshPtrs;
	curMeshPtrs.reserve(node.mNumMeshes);
	for (size_t i = 0; i < node.mNumMeshes; ++i)
	{
		const auto meshIdx = node.mMeshes[i];
		curMeshPtrs.push_back(meshPtrs.at(meshIdx).get());
	}

	auto pNode = std::make_unique<ModelPart>(nextId++, node.mName.C_Str(), std::move(curMeshPtrs), transform);
	for (size_t i = 0; i < node.mNumChildren; ++i)
	{
		pNode->AddChild(ParseNode(nextId, *node.mChildren[i], scale));
	}

	return pNode;
}
