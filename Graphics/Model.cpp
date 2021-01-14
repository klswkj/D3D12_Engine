#include "stdafx.h"
#include "Model.h"
#include "UAVBuffer.h"
#include "Mesh.h"
#include "Material.h"
#include "ModelComponentWindow.h"
#include "MasterRenderGraph.h"
#include "FundamentalVertexIndex.h"
#include "ObjectFilterFlag.h"

// #define VERTEX_LAYOUT_BYTE_SIZE 0xeull * 0x4ull
constexpr size_t FLOAT_SIZE = sizeof(float);
constexpr size_t VERTEX_LAYOUT_BYTE_SIZE = 0xeull * FLOAT_SIZE;
constexpr size_t INDEX_LAYOUT_BYTE_SIZE = sizeof(uint16_t);

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

	const aiScene* pScene = AssImporter.ReadFile
	(
		pathString,
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

	float*    pVertexData = nullptr;
	uint16_t* pIndexData  = nullptr;
	MeshData* pMeshData   = nullptr;
	size_t VertexIndexCount[2];

	LoadMesh(pScene, &pVertexData, &pIndexData, &pMeshData, VertexIndexCount, scale);

	custom::StructuredBuffer* pVertexBuffer =
		custom::StructuredBuffer::CreateVertexBuffer(StringToWString(m_ModelName + "'s_VB"), (uint32_t)VertexIndexCount[0], VERTEX_LAYOUT_BYTE_SIZE, pVertexData);

	custom::ByteAddressBuffer* pIndexBuffer 
		= custom::ByteAddressBuffer::CreateIndexBuffer(StringToWString(m_ModelName + "'s_IB"), (uint32_t)VertexIndexCount[1], (uint32_t)sizeof(uint16_t), pIndexData);

	D3D12_VERTEX_BUFFER_VIEW ModelVBV = pVertexBuffer->VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW  ModelIBV = pIndexBuffer->IndexBufferView();

	for (size_t IMesh = 0; IMesh < pScene->mNumMeshes; ++IMesh)
	{
		const aiMesh&      _aiMesh = *pScene->mMeshes[IMesh];
		const MeshData& __MeshData = pMeshData[IMesh];

		UINT _IndexStartLocation          = (UINT)(__MeshData.IndexDataByteOffset  / sizeof(uint16_t));
		INT _VertexStartLocation          = (INT)(__MeshData.VertexDataByteOffset / VERTEX_LAYOUT_BYTE_SIZE);
		size_t VertexStartLocationInArray = __MeshData.VertexDataByteOffset / sizeof(float);

		FundamentalVertexIndex FVI;
		FVI.VertexBufferView   = ModelVBV;
		FVI.IndexBufferView    = ModelIBV;
		FVI.IndexCount         = __MeshData.IndexCount;
		FVI.StartIndexLocation = _IndexStartLocation;
		FVI.VertexCount        = __MeshData.VertexCount;
		FVI.BaseVertexLocation = _VertexStartLocation;

		m_pMeshes.push_back
		(
			std::make_unique<Mesh>
			(
				materials[_aiMesh.mMaterialIndex],
				FVI,
				pVertexData + VertexStartLocationInArray
			)
		);
	}

	delete[] pVertexData;
	delete[] pIndexData;
	delete[] pMeshData;

	ComputeBoundingBox();

	uint32_t nextId = 0;
	m_pModelPart = ParseNode(nextId, *pScene->mRootNode, scale);
}

void Model::Submit(eObjectFilterFlag Filter) const DEBUG_EXCEPT
{
	m_pModelPart->Submit(Filter, Math::Matrix4(Math::EIdentityTag::kIdentity));
}

void Model::SetRootTransform(Math::Matrix4 _Transform) noexcept
{
	m_pModelPart->SetAppliedTransform(_Transform);
}

void Model::TraverseNode(ModelComponentWindow& _ComponentWindow)
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
	auto ScaleTranslation = [&](Math::Matrix4 _Matrix, float scale)
	{
		float x = _Matrix.GetW().GetX() * scale;
		float y = _Matrix.GetW().GetY() * scale;
		float z = _Matrix.GetW().GetZ() * scale;
		float w = _Matrix.GetW().GetW();

		_Matrix.SetW(Math::Vector4(x, y, z, w));

		return _Matrix;
	};

	/*
	const Math::Matrix4 TransformMatrix = ScaleTranslation
	(
		DirectX::XMMatrixTranspose
		(
			DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&_aiNode.mTransformation))
		), _Scale
	);
	*/

	Math::Matrix4 aiNodeTransformation = Math::Matrix4(DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&_aiNode.mTransformation)));
	// const Math::Matrix4 TransformMatrix = ScaleTranslation(aiNodeTransformation.GetTranspose(), _Scale);
	const Math::Matrix4 TransformMatrix = ScaleTranslation(aiNodeTransformation, _Scale);

	std::vector<Mesh*> pCurrentMeshes;
	pCurrentMeshes.reserve(_aiNode.mNumMeshes);

	for (size_t IMesh = 0; IMesh < _aiNode.mNumMeshes; ++IMesh)
	{
		const size_t CurrentMesh = _aiNode.mMeshes[IMesh];
		pCurrentMeshes.push_back(m_pMeshes[CurrentMesh].get());
	}

	auto pNode = std::make_unique<ModelPart>(NextId++, _aiNode.mName.C_Str(), std::move(pCurrentMeshes), TransformMatrix);

	for (size_t IChild = 0; IChild < _aiNode.mNumChildren; ++IChild)
	{
		pNode->AddModelPart(ParseNode(NextId, *_aiNode.mChildren[IChild], _Scale));
	}

	return pNode;
}

void Model::ComputeBoundingBox()
{
	for (size_t IMesh = 0; IMesh < m_pMeshes.size(); ++IMesh)
	{
		BoundingBox& temp = (BoundingBox&)m_pMeshes[IMesh]->GetBoundingBox();

		m_BoundingBox.MinPoint = Math::Min(temp.MinPoint, m_BoundingBox.MinPoint);
		m_BoundingBox.MaxPoint = Math::Max(temp.MaxPoint, m_BoundingBox.MaxPoint);
	}
}

void Model::LoadMesh(const aiScene* _aiScene, float** pVertices, uint16_t** pIndices, MeshData** pMeshDataArray, size_t VertexIndexCount[], float Scale)
{
	ASSERT(_aiScene->HasMeshes());
	ASSERT(_aiScene->mMeshes[0]->HasPositions());
	ASSERT(_aiScene->mMeshes[0]->HasTextureCoords(0));
	ASSERT(_aiScene->mMeshes[0]->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

	const size_t TotalMeshCount = _aiScene->mNumMeshes;

	ASSERT(TotalMeshCount != 0);

	// TotalMesh 0x189 <-> 0x22 
	*pMeshDataArray = new MeshData[TotalMeshCount];

#ifdef _DEBUG
	// ZeroMemory(&(*pMeshDataArray)[0], sizeof(MeshData) * TotalMeshCount);
	memset(*pMeshDataArray, -1, sizeof(MeshData) * TotalMeshCount);
#endif

	size_t VertexDataByteOffset = 0;
	size_t IndexDataByteOffset  = 0;
	size_t WholeVertexCount     = 0;
	size_t WholeIndexCount      = 0;

	const aiMesh** ppMeshArray = (const aiMesh**)_aiScene->mMeshes;

	for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
	{
		MeshData& CurrentMeshData = (*pMeshDataArray)[IMesh];

		CurrentMeshData.VertexDataByteOffset = VertexDataByteOffset;
		CurrentMeshData.VertexCount          = ppMeshArray[IMesh]->mNumVertices;

		// Assume All faces are indexed Triangle_list.
		CurrentMeshData.IndexDataByteOffset = IndexDataByteOffset;
		CurrentMeshData.IndexCount          = (UINT)ppMeshArray[IMesh]->mNumFaces * 3;

		VertexDataByteOffset += VERTEX_LAYOUT_BYTE_SIZE * CurrentMeshData.VertexCount;
		IndexDataByteOffset  += INDEX_LAYOUT_BYTE_SIZE  * CurrentMeshData.IndexCount;

		WholeVertexCount += CurrentMeshData.VertexCount;
		WholeIndexCount  += CurrentMeshData.IndexCount;
	}

	*pVertices = new float[VertexDataByteOffset / sizeof(float)];
	*pIndices = new uint16_t[IndexDataByteOffset / sizeof(uint16_t)];

#ifdef _DEBUG
	memset(*pVertices, -1, VertexDataByteOffset);
	memset(*pIndices,  -1, IndexDataByteOffset);
#endif
	{
		uint16_t* pIndicesTarget = *pIndices;

		for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
		{
			// ASSERT(ppMeshArray[IMesh]->mFaces[0].mNumIndices == 3);
			const aiMesh* _aiMesh = ppMeshArray[IMesh];

			for (size_t IFaces = 0; IFaces < _aiMesh->mNumFaces; ++IFaces)
			{
				ASSERT(_aiMesh->mFaces[IFaces].mNumIndices == 3);

				*pIndicesTarget++ = _aiMesh->mFaces[IFaces].mIndices[0];
				*pIndicesTarget++ = _aiMesh->mFaces[IFaces].mIndices[1];
				*pIndicesTarget++ = _aiMesh->mFaces[IFaces].mIndices[2];
			}
		}

		ASSERT(*pIndices + IndexDataByteOffset/sizeof(uint16_t) == pIndicesTarget);
	}

	const bool bHasNormal                = ppMeshArray[0]->HasNormals();
	const bool bHasTangentsAndBitangents = ppMeshArray[0]->HasTangentsAndBitangents();

	if (bHasNormal && bHasTangentsAndBitangents)
	{
		float* pVertexTarget = *pVertices;

		for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
		{
			const aiMesh* pMesh       = ppMeshArray[IMesh];
			const size_t mNumVertices = pMesh->mNumVertices;

			for (size_t IVertex = 0; IVertex < mNumVertices; ++IVertex)
			{
				*pVertexTarget++ = pMesh->mVertices[IVertex].x * Scale;
				*pVertexTarget++ = pMesh->mVertices[IVertex].y * Scale;
				*pVertexTarget++ = pMesh->mVertices[IVertex].z * Scale;

				*pVertexTarget++ = pMesh->mNormals[IVertex].x;
				*pVertexTarget++ = pMesh->mNormals[IVertex].y;
				*pVertexTarget++ = pMesh->mNormals[IVertex].z;

				*pVertexTarget++ = pMesh->mTextureCoords[0][IVertex].x;
				*pVertexTarget++ = pMesh->mTextureCoords[0][IVertex].y;

				*pVertexTarget++ = pMesh->mTangents[IVertex].x;
				*pVertexTarget++ = pMesh->mTangents[IVertex].y;
				*pVertexTarget++ = pMesh->mTangents[IVertex].z;

				*pVertexTarget++ = pMesh->mBitangents[IVertex].x;
				*pVertexTarget++ = pMesh->mBitangents[IVertex].y;
				*pVertexTarget++ = pMesh->mBitangents[IVertex].z;
			}
		}

		ASSERT(pVertexTarget == *pVertices + VertexDataByteOffset / sizeof(float));
	}
	else // Compute Normal or Tangent, Bitangent or both of them.
	{
		// Orderby Position, Texcoord, Normal, Tangent, Bitangent
		//     Not Positon, Normal, Texcoord order (This order in Sync with HLSL.)
		
		// Array pTempVertexBegin

		// 0             WholeVertex * 3 - 1
		// ┌──────────────────────┬──────────────────────┬─────┐
		// │ All Meshes' Position │ All Meshes' Texcoord │ ... │
		// └──────────────────────┴──────────────────────┴─────┘

		float* pTempVertexBegin = new float[VertexDataByteOffset / sizeof(float)];

		const float* pTempPosition  = &pTempVertexBegin[0];
		const float* pTempTexcoord0 = pTempPosition  + WholeVertexCount * 3;
		const float* pTempNormal    = pTempTexcoord0 + WholeVertexCount * 2;
		const float* pTempTangent   = pTempNormal    + WholeVertexCount * 3;
		const float* pTempBitangent = pTempTangent   + WholeVertexCount * 3;

		float* pTempTempPosition  = nullptr;
		float* pTempTempTexcoord0 = nullptr;
		float* pTempTempNormal    = nullptr;
		float* pTempTempTangent   = nullptr;
		float* pTempTempBitangent = nullptr;

		// Set All Positions in contiguous array.
		{
			pTempTempPosition = (float*)pTempPosition;

			for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
			{
				const aiMesh* pMesh = ppMeshArray[IMesh];
				memcpy_s
				(
					pTempTempPosition,    (size_t)pMesh->mNumVertices * 3 * sizeof(float),
					&pMesh->mVertices[0], (size_t)pMesh->mNumVertices * 3 * sizeof(float)
				);

				pTempTempPosition += (size_t)pMesh->mNumVertices * 3;
			}

			if (Scale != 1.0f)
			{
				float* pTempTempTempPosition = (float*)pTempPosition;

				while (pTempTempPosition < pTempTempTempPosition)
				{
					*pTempTempTempPosition++ *= Scale;
					*pTempTempTempPosition++ *= Scale;
					*pTempTempTempPosition++ *= Scale;
				}
			}
		}
		// Texcoord0 부분 테스트 제대로해야됨.
		{
			pTempTempTexcoord0 = (float*)pTempTexcoord0;

			for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
			{
				const aiMesh* pMesh = ppMeshArray[IMesh];
				
				for (size_t IVertex = 0; IVertex < pMesh->mNumVertices; ++IVertex)
				{
					*pTempTempTexcoord0++ = pMesh->mTextureCoords[0][IVertex].x;
					*pTempTempTexcoord0++ = pMesh->mTextureCoords[0][IVertex].y;
				}

				// pTempTempPosition += pMesh->mNumVertices * 2;
			}

			ASSERT(pTempTempTexcoord0 == pTempTexcoord0 + WholeVertexCount * 2);
		}
		{
			if (bHasNormal)
			{
				pTempTempNormal = (float*)pTempNormal;

				for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
				{
					const aiMesh* pMesh = ppMeshArray[IMesh];
					const size_t NumVertices = pMesh->mNumVertices;
					memcpy_s
					(
						pTempTempNormal, NumVertices * 3 * sizeof(float),
						&pMesh->mNormals[0], NumVertices * 3 * sizeof(float)
					);

					pTempTempNormal += NumVertices * 3;
				}

				ASSERT(pTempTempNormal == pTempNormal + WholeVertexCount * 3);
			}
			else // if it doesn't have Normal Vertex, compute it.
			{
				ASSERT(WholeIndexCount % 3 == 0);

				const size_t WholeFaceCount = WholeIndexCount / 3;

				ASSERT_HR
				(
					DirectX::ComputeNormals
					(
						(uint16_t*)&pIndices[0],
						WholeFaceCount,
						(DirectX::XMFLOAT3*)(&pTempTempPosition[0]), 
						WholeVertexCount,
						DirectX::CNORM_DEFAULT,
						(DirectX::XMFLOAT3*)(&pTempNormal[0])
					)
				);
				ASSERT(false, "Debug this.");
			}

			if (bHasTangentsAndBitangents)
			{
				// Make Tangent Vertices
				{
					pTempTempTangent = (float*)pTempTangent;

					for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
					{
						const aiMesh* pMesh = ppMeshArray[IMesh];
						const size_t NumVertices = pMesh->mNumVertices;
						memcpy_s
						(
							pTempTempTangent, NumVertices * 3 * sizeof(float),
							&pMesh->mTangents[0], NumVertices * 3 * sizeof(float)
						);

						pTempTempTangent += NumVertices * 3;
					}

					ASSERT(pTempTempTangent == pTempTangent + WholeVertexCount * 3);
				}
				// Make Bitangent vertices
				{
					pTempTempBitangent = (float*)pTempBitangent;

					for (size_t IMesh = 0; IMesh < TotalMeshCount; ++IMesh)
					{
						const aiMesh* pMesh = ppMeshArray[IMesh];
						const size_t NumVertices = pMesh->mNumVertices;
						memcpy_s
						(
							pTempTempBitangent, NumVertices * 3 * sizeof(float),
							&pMesh->mTangents[0], NumVertices * 3 * sizeof(float)
						);

						pTempTempBitangent += NumVertices * 3;
					}

					ASSERT(pTempTempBitangent == pTempBitangent + WholeVertexCount * 3);
				}
			}
			else
			{
				ASSERT(WholeIndexCount % 3 == 0);

				const size_t WholeFaceCount = WholeIndexCount / 3;

				ASSERT_HR
				(
					DirectX::ComputeTangentFrame
					(
						(uint16_t*)&pIndices[0], WholeFaceCount,
						(DirectX::XMFLOAT3*)(&pTempPosition [0]),
						(DirectX::XMFLOAT3*)(&pTempNormal   [0]),
						(DirectX::XMFLOAT2*)(&pTempTexcoord0[0]), WholeVertexCount,
						(DirectX::XMFLOAT3*)(&pTempTangent  [0]),
						(DirectX::XMFLOAT3*)(&pTempBitangent[0])
					)
				);
				ASSERT(false, "Debug this.");
			}
		}

		pTempTempPosition  = (float*)pTempPosition;
		pTempTempNormal    = (float*)pTempNormal;
		pTempTempTexcoord0 = (float*)pTempTexcoord0;
		pTempTempTangent   = (float*)pTempTangent;
		pTempTempBitangent = (float*)pTempBitangent;

		float* pVertexTarget = *pVertices;

		for (size_t IVertex = 0; IVertex < WholeVertexCount; ++IVertex)
		{
			*pVertexTarget++ = *pTempTempPosition++; // x
			*pVertexTarget++ = *pTempTempPosition++; // y
			*pVertexTarget++ = *pTempTempPosition++; // z

			*pVertexTarget++ = *pTempTempNormal++;
			*pVertexTarget++ = *pTempTempNormal++;
			*pVertexTarget++ = *pTempTempNormal++;

			*pVertexTarget++ = *pTempTempTexcoord0++; // x
			*pVertexTarget++ = *pTempTempTexcoord0++; // y

			*pVertexTarget++ = *pTempTempTangent++;
			*pVertexTarget++ = *pTempTempTangent++;
			*pVertexTarget++ = *pTempTempTangent++;

			*pVertexTarget++ = *pTempTempBitangent++;
			*pVertexTarget++ = *pTempTempBitangent++;
			*pVertexTarget++ = *pTempTempBitangent++;
		}

		ASSERT(pVertexTarget == *pVertices + VertexDataByteOffset / sizeof(float));
		delete[] pTempVertexBegin;
	}

	// delete[] pMeshDataArray;
	VertexIndexCount[0] = WholeVertexCount;
	VertexIndexCount[1] = WholeIndexCount;
}

void Model::OptimizeRemoveDuplicateVertices(MeshData* pMeshData, float* pVertices, uint16_t* pIndices, size_t NumVertices, size_t NumIndices, size_t& TotalVertexCount, size_t& TotalMeshCount)
{
	// 필요한 것
    // Total Vertex Count.
    // Total Mesh Count.
}
