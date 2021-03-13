#include "stdafx.h"
#include "Entity.h"
// #include "Mesh.h"
#include "Material.h"
#include "CommandContext.h"
#include "FundamentalVertexIndex.h"

IEntity::IEntity(const Material& CMaterial, FundamentalVertexIndex& Input, const float* pStartVertexLocation, std::string MeshName)
	:
#if defined(_DEBUG)
	m_name(MeshName),
#endif
	m_Topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
	ASSERT(Input.BaseVertexLocation != -1);
	ASSERT(Input.IndexCount         != -1);
	ASSERT(Input.StartIndexLocation != -1);
	ASSERT(Input.VertexCount        != -1);

	ComputeBoundingBox(pStartVertexLocation, Input.VertexCount, 0xe);

	m_IndexCount         = Input.IndexCount;
	m_StartIndexLocation = Input.StartIndexLocation;
	m_BaseVertexLocation = Input.BaseVertexLocation;
	m_VertexBufferView   = Input.VertexBufferView;
	m_IndexBufferView    = Input.IndexBufferView;

	for (auto& _Technique : CMaterial.GetTechniques())
	{
		AddTechnique(std::move(_Technique));
	}
}

void IEntity::AddTechnique(Technique _Technique) noexcept
{
	_Technique.InitializeParentReferences(*this);
	m_Techniques.push_back(std::move(_Technique));
}

void IEntity::Submit(eObjectFilterFlag Filter) noexcept
{
	for (std::vector<Technique>::iterator::value_type& Tech : m_Techniques)
	{
		Tech.Submit(*this, Filter);
	}
}

void IEntity::Bind(custom::CommandContext& BaseContext, uint8_t commandIndex) const DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetVertexBuffer(0, m_VertexBufferView, commandIndex);
	graphicsContext.SetIndexBuffer(m_IndexBufferView, commandIndex);
	// graphicsContext.SetPrimitiveTopology(m_Topology);
}

void IEntity::Accept(IWindow& window)
{
	for (auto& Tech : m_Techniques)
	{
		Tech.Accept(window); // For Outline Technique
	}
}

void IEntity::LinkTechniques(const MasterRenderGraph& _MasterRenderGraph)
{
	for (auto& tech : m_Techniques)
	{
		tech.Link(_MasterRenderGraph);
	}
}

// Must keep in sync with HLSL
void IEntity::LoadMesh(const aiMesh& _aiMesh, VertexLayout** pVertexLayout, uint16_t** pIndices)
{
	ASSERT(_aiMesh.HasPositions());
	ASSERT(_aiMesh.HasTextureCoords(0));
	ASSERT(_aiMesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

	// TODO : make to custom::make_scopedptr
	*pVertexLayout = new VertexLayout[_aiMesh.mNumVertices];
	*pIndices = new uint16_t[_aiMesh.mNumFaces * 3];

	for (size_t i = 0; i < _aiMesh.mNumFaces; ++i)
	{
		(*pIndices)[i * 3] = _aiMesh.mFaces[i].mIndices[0];
		(*pIndices)[i * 3 + 1] = _aiMesh.mFaces[i].mIndices[1];
		(*pIndices)[i * 3 + 2] = _aiMesh.mFaces[i].mIndices[2];
	}

	const size_t numVertices = _aiMesh.mNumVertices;
	const uint32_t VertexStride = sizeof(VertexLayout);

	// Orderby Position, Texcoord, Normal, Tangent, BiTangent
	//     Not Position, Normal, Texcoord, ... 
	float* pTempVertexBegin = new float[numVertices * VertexStride / 4];
	float* pTempPosition = &pTempVertexBegin[0];
	float* pTempTexcoord = pTempPosition + numVertices * 3;
	float* pTempNormal = pTempTexcoord + numVertices * 2;
	float* pTempTangnet = pTempNormal + numVertices * 3;
	float* pTempBitanget = pTempTangnet + numVertices * 3;

	// ZeroMemory(&pTempVertexBegin, numVertices * VertexStride / 4);

	memset(pTempVertexBegin, -1, numVertices * VertexStride);

	constexpr size_t kFloat2 = sizeof(float) * 2;
	constexpr size_t kFloat3 = sizeof(float) * 3;

	// copy Position Elements from aimesh
	memcpy_s(&pTempPosition[0], kFloat3 * numVertices, &_aiMesh.mVertices[0], kFloat3 * numVertices);

	ComputeBoundingBox(pTempPosition, numVertices);

	// copy Texcoord Elements from aimesh
	for (size_t i = 0; i < numVertices; ++i)
	{
		pTempTexcoord[i * 2] = _aiMesh.mTextureCoords[0][i].x;
		pTempTexcoord[i * 2 + 1] = _aiMesh.mTextureCoords[0][i].y;
	}

	if (_aiMesh.HasNormals())
	{
		memcpy_s(&pTempNormal[0], kFloat3 * numVertices, &_aiMesh.mNormals[0], kFloat3 * numVertices);
	}
	else
	{
		ASSERT_HR
		(
			DirectX::ComputeNormals
			(
				(uint16_t*)&pIndices[0], _aiMesh.mNumFaces,
				reinterpret_cast<DirectX::XMFLOAT3*>(&pTempPosition[0]), numVertices,
				DirectX::CNORM_DEFAULT,
				reinterpret_cast<DirectX::XMFLOAT3*>(&pTempNormal[0])
			)
		);
	}

	if (_aiMesh.HasTangentsAndBitangents())
	{
		memcpy_s(&pTempTangnet[0], kFloat3 * numVertices, &_aiMesh.mTangents[0], kFloat3 * numVertices);
		memcpy_s(&pTempBitanget[0], kFloat3 * numVertices, &_aiMesh.mBitangents[0], kFloat3 * numVertices);
	}
	else
	{
		ASSERT_HR
		(
			DirectX::ComputeTangentFrame
			(
				(uint16_t*)&pIndices[0], _aiMesh.mNumFaces,
				reinterpret_cast<DirectX::XMFLOAT3*>(&pTempPosition[0]),
				reinterpret_cast<DirectX::XMFLOAT3*>(&pTempNormal[0]),
				reinterpret_cast<DirectX::XMFLOAT2*>(&pTempTexcoord[0]), numVertices,
				reinterpret_cast<DirectX::XMFLOAT3*>(&pTempTangnet[0]),
				reinterpret_cast<DirectX::XMFLOAT3*>(&pTempBitanget[0])
			)
		);
	}

	for (size_t i = 0; i < numVertices; ++i)
	{
		(*pVertexLayout)[i].Position[0] = pTempPosition[i * 3];
		(*pVertexLayout)[i].Position[1] = pTempPosition[i * 3 + 1];
		(*pVertexLayout)[i].Position[2] = pTempPosition[i * 3 + 2];

		(*pVertexLayout)[i].Normal[0] = pTempNormal[i * 3];
		(*pVertexLayout)[i].Normal[1] = pTempNormal[i * 3 + 1];
		(*pVertexLayout)[i].Normal[2] = pTempNormal[i * 3 + 2];

		(*pVertexLayout)[i].TexCoord[0] = pTempTexcoord[i * 3];
		(*pVertexLayout)[i].TexCoord[1] = pTempTexcoord[i * 3 + 1];

		(*pVertexLayout)[i].Tangent[0] = pTempTangnet[i];
		(*pVertexLayout)[i].Tangent[1] = pTempTangnet[i * 3 + 1];
		(*pVertexLayout)[i].Tangent[2] = pTempTangnet[i * 3 + 2];

		(*pVertexLayout)[i].Bitangent[0] = pTempBitanget[i];
		(*pVertexLayout)[i].Bitangent[1] = pTempBitanget[i * 3 + 1];
		(*pVertexLayout)[i].Bitangent[2] = pTempBitanget[i * 3 + 2];
	}
#ifdef _DEBUG
	std::string fileOutName = "../EntityDebug/Mesh_";

	static uint64_t DebugNumber = 0;

	fileOutName += std::to_string(DebugNumber);
	fileOutName += "_Vertex";
	fileOutName += ".txt";

	std::ofstream fileOut(fileOutName);

	fileOut << "Vertices" << std::endl;

	for (size_t i = 0ul; i < _aiMesh.mNumVertices; ++i)
	{
		fileOut << "Position  " << i << " : " << (*pVertexLayout)[i].Position[0] << ", " << (*pVertexLayout)[i].Position[1] << ", " << (*pVertexLayout)[i].Position[2] << "\n";
		fileOut << "Normal    " << i << " : " << (*pVertexLayout)[i].Normal[0] << ", " << (*pVertexLayout)[i].Normal[1] << ", " << (*pVertexLayout)[i].Normal[2] << "\n";
		fileOut << "Texcoord  " << i << " : " << (*pVertexLayout)[i].TexCoord[0] << ", " << (*pVertexLayout)[i].TexCoord[1] << "\n";
		fileOut << "Tangent   " << i << " : " << (*pVertexLayout)[i].Tangent[0] << ", " << (*pVertexLayout)[i].Tangent[1] << ", " << (*pVertexLayout)[i].Tangent[2] << "\n";
		fileOut << "BiTangnet " << i << " : " << (*pVertexLayout)[i].Bitangent[0] << ", " << (*pVertexLayout)[i].Bitangent[1] << ", " << (*pVertexLayout)[i].Bitangent[2] << "\n\n";
	}

	fileOut.close();
	fileOutName.clear();

	fileOutName += "../EntityDebug/Mesh_";
	fileOutName += std::to_string(DebugNumber);
	fileOutName += "_Index.txt";

	fileOut.open(fileOutName);

	for (size_t i = 0ul; i < _aiMesh.mNumFaces; ++i)
	{
		fileOut << "Faces #" << i << " : " << (*pIndices)[i * 3] << ", " << (*pIndices)[i * 3 + 1] << ", " << (*pIndices)[i * 3 + 2] << std::endl;
	}

	++DebugNumber;
#endif
	delete[] pTempVertexBegin;
}

void IEntity::ComputeBoundingBox(const float* pVertices, size_t NumVertices, size_t VertexStrideCount)
{
	if (0 < NumVertices)
	{
		Math::Vector3 TempVertex;

		for (size_t i = 0; i < NumVertices; ++i)
		{
			TempVertex = Math::Vector3(pVertices[i * VertexStrideCount], pVertices[i * VertexStrideCount + 1], pVertices[i * VertexStrideCount + 2]);

			m_BoundingBox.MinPoint = Math::Min(TempVertex, m_BoundingBox.MinPoint);
			m_BoundingBox.MaxPoint = Math::Max(TempVertex, m_BoundingBox.MaxPoint);
		}
	}
}


/*
// TODO : Make Bounding Box
IEntity::IEntity(const Material& CMaterial, const aiMesh& _aiMesh, float _Scale) noexcept
	: m_BoundingBox({ Math::Scalar(0.0f), Math::Scalar(0.0f) })
{
	VertexLayout* pVertexLayout = nullptr;
	uint16_t* pIndices = nullptr;

	LoadMesh(_aiMesh, &pVertexLayout, &pIndices);

	ASSERT(pVertexLayout != nullptr);
	ASSERT(pIndices != nullptr);

	m_VerticesBuffer.Create(AnsiToWString(_aiMesh.mName.C_Str()) + L" Vertex Buffer", _aiMesh.mNumVertices, sizeof(VertexLayout), pVertexLayout);
	m_IndicesBuffer.Create(AnsiToWString(_aiMesh.mName.C_Str()) + L" Index Buffer", _aiMesh.mNumFaces * 3, sizeof(uint16_t), pIndices);

	delete[] pVertexLayout;
	pVertexLayout = nullptr;
	delete[] pIndices;
	pIndices = nullptr;

	for (auto& _Technique : CMaterial.GetTechniques())
	{
		AddTechnique(std::move(_Technique));
	}
}
*/
/*
IEntity::IEntity(const Material& CMaterial, const aiMesh& _aiMesh, float _Scale) noexcept
{
	// unsigned int mNumVertices -> size of per-vertex data arrays
	// unsigned int mNumFaces    -> size of mFaces array
	// aiVector3D* mVertices     -> Vertex Position
	// aiVector3D* mNormals      -> Vertex Normal
	// aiVector3D* mTangents     -> Vertex Tangent
	// aiVector3D* mBiTangents   -> Vertex BiTangents

	{
		uint32_t numVertexElements = 0;
		uint32_t vertexStrideSize = 0;

		static constexpr uint32_t FLOAT_SIZE = sizeof(float);

		std::vector<std::pair<float*, size_t>> VertexElements;
		VertexElements.reserve(5);

		bool bComponents[4] = { false };

		// Shader Code 내에서도 이 순서 지켜야됨.

		bool& bPosition = bComponents[0];
		bool& bTexcoord = bComponents[1];
		bool& bNormal   = bComponents[2];
		bool& bTangents = bComponents[3];

		if (_aiMesh.HasPositions())
		{
			VertexElements.push_back({ &_aiMesh.mVertices[0].x, 3 });
			bPosition = true;
			vertexStrideSize += FLOAT_SIZE * 3; // += POSITION
			++numVertexElements;
		}
		if (_aiMesh.HasNormals())
		{
			VertexElements.push_back({ &_aiMesh.mNormals[0].x, 3 });
			bNormal = true;

			vertexStrideSize += FLOAT_SIZE * 3; // += Texcoord, Normal
			++numVertexElements;
		}
		if (_aiMesh.HasTextureCoords(0))
		{
			VertexElements.push_back({ &_aiMesh.mTextureCoords[0]->x, 2 });
			bTexcoord = true;
			vertexStrideSize += FLOAT_SIZE * 2; // += Texcoord, Normal
			++numVertexElements;
		}
		if (_aiMesh.HasTangentsAndBitangents())
		{
			VertexElements.push_back({ &_aiMesh.mTangents[0].x, 3 });
			VertexElements.push_back({ &_aiMesh.mBitangents[0].x, 3 });
			bTexcoord = true;
			bTangents = true;
			vertexStrideSize += FLOAT_SIZE * 6; // += Texcoord, Tangents, Bitangents
			numVertexElements += 3;
		}

		float* pVertices = (float*)malloc(_aiMesh.mNumVertices * vertexStrideSize);
		memset(pVertices, 0, _aiMesh.mNumVertices * vertexStrideSize); // Zeromemory(pVertices[0], _aiMesh.mNumVertices * vertexStrideSize

		const uint64_t maxSize = _aiMesh.mNumVertices * numVertexElements * FLOAT_SIZE;

		{
			size_t numVertices = _aiMesh.mNumVertices;

			// Position   3f, bPosition
			// Texcoord   2f, bTexcoord
			// Normal     3f, bNormal
			// Tangents   3f, bTangents
			// biTangents 3f, bTangents

			size_t current = 0; // In Vertices Index

			for (size_t IVertexElement = 0; IVertexElement < VertexElements.size(); ++IVertexElement)
			{
				size_t jumpSize = (vertexStrideSize / FLOAT_SIZE);

				current = 0;

				for (size_t i = 0; i < IVertexElement; ++i)
				{
					current += VertexElements[i].second;
				}

				{
					const rsize_t sourceSize = VertexElements[IVertexElement].second * FLOAT_SIZE;

					for (size_t vertex = 0; vertex < numVertices; ++vertex)
					{
						memcpy_s(&pVertices[current], sourceSize, VertexElements[IVertexElement].first, sourceSize);
						VertexElements[IVertexElement].first += VertexElements[IVertexElement].second;
						current += jumpSize;
					}

					if (IVertexElement == 0 && _Scale != 1.0f)
					{
						for (size_t IVertex = 0; IVertex < numVertices; ++IVertex)
						{
							pVertices[jumpSize * IVertex] *= _Scale;
							pVertices[jumpSize * IVertex + 1] *= _Scale;
							pVertices[jumpSize * IVertex + 2] *= _Scale;
						}
					}
				}
			}
		}

		m_VerticesBuffer.Create(AnsiToWString(_aiMesh.mName.C_Str()) + L" Vertex Buffer", _aiMesh.mNumVertices, vertexStrideSize, pVertices);

		delete[] pVertices;
		pVertices = nullptr;
	} // End VertexBuffer

	if (_aiMesh.HasFaces())
	{
		// 여기 잘못 됨
		m_IndicesBuffer.Create(AnsiToWString(_aiMesh.mName.C_Str()) + L" Index Buffer", _aiMesh.mNumFaces, sizeof(unsigned short) * 3, (void*)_aiMesh.mFaces);
	}

	for (auto& Tech : CMaterial.GetTechniques())
	{
		AddTechnique(std::move(Tech));
	}
}
*/
/*
#include "ModelLoader.h"
#include <unordered_map>
#include "DirectXMesh.h"

using namespace DirectX;

void CalculateTangents(Vertex* vertices, UINT vertexCount, uint32* indices, UINT indexCount)
{
	XMFLOAT3* tan1 = new XMFLOAT3[vertexCount * 2];
	XMFLOAT3* tan2 = tan1 + vertexCount;
	ZeroMemory(tan1, vertexCount * sizeof(XMFLOAT3) * 2);
	int triangleCount = indexCount / 3;
	for (UINT i = 0; i < indexCount; i += 3)
	{
		int i1 = indices[i];
		int i2 = indices[i + 2];
		int i3 = indices[i + 1];
		auto v1 = vertices[i1].Position;
		auto v2 = vertices[i2].Position;
		auto v3 = vertices[i3].Position;

		auto w1 = vertices[i1].UV;
		auto w2 = vertices[i2].UV;
		auto w3 = vertices[i3].UV;

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;
		float r = 1.0F / (s1 * t2 - s2 * t1);

		XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r);

		XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r);

		XMStoreFloat3(&tan1[i1], XMLoadFloat3(&tan1[i1]) + XMLoadFloat3(&sdir));
		XMStoreFloat3(&tan1[i2], XMLoadFloat3(&tan1[i2]) + XMLoadFloat3(&sdir));
		XMStoreFloat3(&tan1[i3], XMLoadFloat3(&tan1[i3]) + XMLoadFloat3(&sdir));

		XMStoreFloat3(&tan2[i1], XMLoadFloat3(&tan2[i1]) + XMLoadFloat3(&tdir));
		XMStoreFloat3(&tan2[i2], XMLoadFloat3(&tan2[i2]) + XMLoadFloat3(&tdir));
		XMStoreFloat3(&tan2[i3], XMLoadFloat3(&tan2[i3]) + XMLoadFloat3(&tdir));
	}

	for (UINT a = 0; a < vertexCount; a++)
	{
		auto n = vertices[a].Normal;
		auto t = tan1[a];

		// Gram-Schmidt orthogonalize
		auto dot = XMVector3Dot(XMLoadFloat3(&n), XMLoadFloat3(&t));
		XMStoreFloat3(&vertices[a].Tangent, XMVector3Normalize(XMLoadFloat3(&t) - XMLoadFloat3(&n) * dot));

		// Calculate handedness
		//vertices[a].Tangent.w = (XMVectorGetX(XMVector3Dot(XMVector3Cross(XMLoadFloat3(&n), XMLoadFloat3(&t)), XMLoadFloat3(&tan2[a]))) < 0.0F) ? -1.0F : 1.0F;
	}

	delete[] tan1;
}

void ProcessMesh(UINT index, aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32>& indices)
{
	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.Position.x = mesh->mVertices[i].x;
		vertex.Position.y = mesh->mVertices[i].y;
		vertex.Position.z = mesh->mVertices[i].z;

		if (mesh->HasNormals())
		{
			vertex.Normal = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		}

		if (mesh->mTextureCoords[0])
		{
			vertex.UV.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.UV.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	if (!mesh->HasNormals() || !mesh->HasTangentsAndBitangents())
	{
		std::vector<XMFLOAT3> pos;
		std::vector<XMFLOAT3> normals;
		std::vector<XMFLOAT3> tangents;
		std::vector<XMFLOAT2> uv;

		for (auto& v : vertices)
		{
			pos.push_back(v.Position);
			normals.push_back(v.Normal);
			tangents.push_back(v.Tangent);
			uv.push_back(v.UV);
		}

		if (!mesh->HasNormals())
		{
			DirectX::ComputeNormals(indices.data(), mesh->mNumFaces, pos.data(), pos.size(), CNORM_DEFAULT, normals.data());
		}

		if (!mesh->HasTangentsAndBitangents())
		{
			DirectX::ComputeTangentFrame(indices.data(), mesh->mNumFaces, pos.data(), normals.data(), uv.data(), pos.size(), tangents.data(), nullptr);
		}

		for (int i = 0; i < vertices.size(); ++i)
		{
			vertices[i].Normal = normals[i];
			vertices[i].Tangent = tangents[i];
		}
	}
}


MeshData ModelLoader::Load(const std::string& filename)
{
	MeshData mesh;
	static Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);

	if (pScene == NULL)
		return mesh;

	uint32 NumVertices = 0;
	uint32 NumIndices = 0;
	std::vector<MeshEntry> meshEntries(pScene->mNumMeshes);
	for (uint32 i = 0; i < meshEntries.size(); i++)
	{
		meshEntries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		meshEntries[i].BaseVertex = NumVertices;
		meshEntries[i].BaseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += meshEntries[i].NumIndices;
	}

	std::vector<Vertex> vertices;
	std::vector<uint32> indices;

	for (uint32 i = 0; i < pScene->mNumMeshes; ++i)
	{
		ProcessMesh(i, pScene->mMeshes[i], pScene, vertices, indices);
	}

	mesh.Vertices = std::move(vertices);
	mesh.Indices = std::move(indices);
	mesh.MeshEntries = meshEntries;
	return mesh;
}

ModelData  ModelLoader::LoadModel(const std::string& filename)
{
	static Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);

	if (pScene == NULL)
		return ModelData{};

	uint32 NumVertices = 0;
	uint32 NumIndices = 0;

	std::vector<Vertex> vertices;
	std::vector<uint32> indices;
	MeshData mesh;
	std::vector<MeshMaterial> materials(pScene->mNumMeshes);

	std::vector<MeshEntry> meshEntries(pScene->mNumMeshes);
	for (uint32 i = 0; i < meshEntries.size(); i++)
	{
		meshEntries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		meshEntries[i].BaseVertex = NumVertices;
		meshEntries[i].BaseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += meshEntries[i].NumIndices;
	}

	for (uint32 i = 0; i < pScene->mNumMeshes; i++)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32> indices;
		ProcessMesh(i, pScene->mMeshes[i], pScene, vertices, indices);
		mesh.Vertices.insert(mesh.Vertices.end(), vertices.begin(), vertices.end());
		mesh.Indices.insert(mesh.Indices.end(), indices.begin(), indices.end());
	}

	//mesh.Vertices = std::move(vertices);
	//mesh.Indices = std::move(indices);
	mesh.MeshEntries = meshEntries;

	if (pScene->HasMaterials())
	{
		for (uint32 i = 0; i < pScene->mNumMeshes; i++)
		{
			auto matId = pScene->mMeshes[i]->mMaterialIndex;
			auto mat = pScene->mMaterials[matId];
			auto c = mat->mNumProperties;
			aiString diffuseTexture;
			aiString normalTexture;
			aiString roughnessTexture;
			aiString metalnessTexture;
			MeshMaterial mMat;
			if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexture) == aiReturn_SUCCESS)
			{
				mMat.Diffuse = diffuseTexture.C_Str();
			}

			if (mat->GetTexture(aiTextureType_NORMALS, 0, &normalTexture) == aiReturn_SUCCESS || mat->GetTexture(aiTextureType_HEIGHT, 0, &normalTexture) == aiReturn_SUCCESS)
			{
				mMat.Normal = normalTexture.C_Str();
			}

			if (mat->GetTexture(aiTextureType_SPECULAR, 0, &roughnessTexture) == aiReturn_SUCCESS)
			{
				mMat.Roughness = roughnessTexture.C_Str();
			}

			if (mat->GetTexture(aiTextureType_AMBIENT, 0, &metalnessTexture) == aiReturn_SUCCESS)
			{
				mMat.Metalness = metalnessTexture.C_Str();
			}

			materials[i] = mMat;
		}
	}

	return ModelData{ mesh, materials };
}

*/