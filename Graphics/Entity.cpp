#include "stdafx.h"
#include "Entity.h"
#include "Mesh.h"
#include "Material.h"
#include "CommandContext.h"
#include <assimp/scene.h>
#include <assimp/mesh.h>

// 나중에 Reslove로 따로 메모리풀처럼 VertexBuffer들은 VertexBuffer끼리 관리하고, 
// 예로들어 Create()로 초기화 시키면, 그 때 해시맵이나 맵에 저장되어있는지 확인하고,
// 같은거면 포인터 알려주고, 아니면 새로만들어서 위의 자료구조에 저장하기.

Entity::Entity(const Material& mat, const aiMesh& mesh, float scale/* = 1.0f*/) noexcept
{
	// aiMesh::mPrimitiveTypes
	// aiPrimitiveType_POINT       = 0x1
	// aiPrimitiveType_LINE        = 0x2
	// aiPrimitiveType_TRIANGLE    = 0x4
	// aiPrimitiveType_POLYGON     = 0x8

	// unsigned int mNumVertices -> size of per-vertex data arrays
	// unsigned int mNumFaces    -> size of mFaces array
	// aiVector3D* mVertices     -> Vertex Position
	// aiVector3D* mNormals      -> Vertex Normal
	// aiVector3D* mTangents     -> Vertex Tangent
	// aiVector3D* mBiTangents   -> Vertex BiTangents

	//================================================================//
	// 일단 Element 사이즈부터 재고, 
	// VertexBuffer
	{
		uint32_t numVertexElements{ 0 };
		uint32_t vertexStrideSize{ 0 };

		static constexpr uint32_t FLOAT_SIZE = sizeof(float);

		std::vector<std::pair<float*, size_t>> Components;
		Components.reserve(5);

		bool bComponents[4] = { false };

		bool& bPosition = bComponents[0];
		bool& bTexcoord = bComponents[1];
		bool& bNormal = bComponents[2];
		bool& bTangents = bComponents[3];

		if (mesh.HasPositions())
		{
			Components.push_back({ &mesh.mVertices[0].x, 3 });
			bPosition = true;
			vertexStrideSize += FLOAT_SIZE * 3; // += POSITION
			++numVertexElements;
		}
		if (mesh.HasNormals())
		{
			Components.push_back({ &mesh.mTextureCoords[0]->x, 2 });
			Components.push_back({ &mesh.mNormals[0].x, 3 });
			bNormal = true;
			bTexcoord = true;
			vertexStrideSize += FLOAT_SIZE * 5; // += Texcoord, Normal
			numVertexElements += 2;

			if (mesh.HasTangentsAndBitangents())
			{
				Components.push_back({ &mesh.mTangents[0].x, 3 });
				Components.push_back({ &mesh.mBitangents[0].x, 3 });
				bTangents = true;
				vertexStrideSize += FLOAT_SIZE * 6; // += Tangents, Bitangents
				numVertexElements += 2;
			}
		}
		else
		{
			if (mesh.HasTangentsAndBitangents())
			{
				Components.push_back({ &mesh.mTextureCoords[0]->x, 2 });
				Components.push_back({ &mesh.mTangents[0].x, 3 });
				Components.push_back({ &mesh.mBitangents[0].x, 3 });
				bTexcoord = true;
				bTangents = true;
				vertexStrideSize += FLOAT_SIZE * 8; // += Texcoord, Tangents, Bitangents
				numVertexElements += 3;
			}
		}

		// float* pVertices = new float[mesh.mNumVertices * numVertexElements];
		float* pVertices = (float*)malloc(mesh.mNumVertices * vertexStrideSize);

		memset(pVertices, 0, mesh.mNumVertices * vertexStrideSize);

		const uint64_t maxSize = mesh.mNumVertices * numVertexElements * FLOAT_SIZE;

		{
			size_t numVertices = mesh.mNumVertices;

			// Position   3f, bPosition
			// Texcoord   2f, bTexcoord
			// Normal     3f, bNormal
			// Tangents   3f, bTangents
			// biTangents 3f, bTangents

			size_t current{ 0 }; // In Vertices Index

			for (size_t component{ 0 }; component < Components.size(); ++component)
			{
				size_t jumpSize = (vertexStrideSize / FLOAT_SIZE);

				current = 0;

				for (size_t i{ 0 }; i < component; ++i)
				{
					current += Components[i].second;
				}

				if (component == 0)
				{
					for (size_t vertex{ 0 }; vertex < numVertices; ++vertex)
					{
						memcpy_s(&pVertices[current], maxSize, Components[component].first, Components[component].second * FLOAT_SIZE);
						Components[component].first = (Components[component].first + Components[component].second + 1);
						current += jumpSize;
					}
					if (scale != 1.0f)
					{
						for (size_t vertex{ 0 }; vertex < numVertices; ++vertex)
						{
							pVertices[jumpSize * vertex] *= scale;
							pVertices[jumpSize * vertex + 1] *= scale;
							pVertices[jumpSize * vertex + 2] *= scale;
						}
					}

				}
				else if (component == 1)
				{
					for (size_t vertex{ 0 }; vertex < numVertices; ++vertex)
					{
						memcpy_s(&pVertices[current], maxSize, Components[component].first, Components[component].second * FLOAT_SIZE);
						Components[component].first = (Components[component].first + Components[component].second + 1);
						current += jumpSize;
					}
				}
				else
				{
					for (size_t vertex{ 0 }; vertex < numVertices; ++vertex)
					{
						memcpy_s(&pVertices[current], maxSize, Components[component].first, Components[component].second * FLOAT_SIZE);
						Components[component].first = (Components[component].first + Components[component].second);
						current += jumpSize;
					}
				}
			}
		}

		m_VerticesBuffer.Create(AnsiToWString(mesh.mName.C_Str()) + L" Vertex Buffer", mesh.mNumVertices, vertexStrideSize, pVertices);

		delete[] pVertices;
		pVertices = nullptr;
	} // VertexBuffer

	// IndexBuffer
	if (mesh.HasFaces())
	{
		unsigned short* Indices = new unsigned short[mesh.mNumFaces * 3];

		size_t IndicesIndex{ 0 };

		for (size_t i{ 0 }; i < mesh.mNumFaces; ++i)
		{
			const aiFace& face = mesh.mFaces[i];
			ASSERT(face.mNumIndices == 3);
			Indices[i * mesh.mNumFaces + 0] = face.mIndices[0];
			Indices[i * mesh.mNumFaces + 1] = face.mIndices[1];
			Indices[i * mesh.mNumFaces + 2] = face.mIndices[2];
		}

		m_IndicesBuffer.Create(AnsiToWString(mesh.mName.C_Str()) + L" Index Buffer", mesh.mNumFaces * 3, sizeof(unsigned short) * 3, Indices);

		delete[] Indices;
		Indices = nullptr;
	} // IndexBuffer
}

void Entity::AddTechnique(Technique Tech) noexcept
{
	Tech.InitializeParentReferences(*this);
	// techniques.push_back(Tech);
	techniques.push_back(std::move(Tech));
}
void Entity::Submit(eObjectFilterFlag Filter) const noexcept
{
	for (const auto& Tech : techniques)
	{
		Tech.Submit(*this, Filter);
	}
}
void Entity::Bind(custom::CommandContext& BaseContext) const DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetVertexBuffer(0, m_VerticesBuffer.VertexBufferView());
	graphicsContext.SetIndexBuffer(m_IndicesBuffer.IndexBufferView());
}
void Entity::Accept(AddonWindow& window)
{
	for (auto& Tech : techniques)
	{
		Tech.Accept(window); // TODO : AddonWindow is not defined.
	}
}
uint32_t Entity::GetIndexCount() const DEBUG_EXCEPT
{
	return m_IndicesBuffer.GetElementCount();
}
void Entity::LinkTechniques(MasterRenderGraph& _MasterRenderGraph)
{
	for (auto& tech : techniques)
	{
		tech.Link(_MasterRenderGraph);
	}
}


/*
{
			uint32_t numVertexElements{ 0 };
			uint32_t vertexStrideSize{ 0 };

			static constexpr uint32_t FLOAT_SIZE = sizeof(float);

			std::vector<std::pair<float*, size_t>> Components;
			Components.reserve(5);

			bool bComponents[4] = { false };

			bool& bPosition = bComponents[0];
			bool& bTexcoord = bComponents[1];
			bool& bNormal = bComponents[2];
			bool& bTangents = bComponents[3];

			if (mesh->HasPositions())
			{
				Components.push_back({ &mesh->mVertices[0].x, 3 });
				bPosition = true;
				vertexStrideSize += FLOAT_SIZE * 3; // += POSITION
				++numVertexElements;
			}
			if (mesh->HasNormals())
			{
				Components.push_back({ &mesh->mTextureCoords[0]->x, 2 });
				Components.push_back({ &mesh->mNormals[0].x, 3 });
				bNormal = true;
				bTexcoord = true;
				vertexStrideSize += FLOAT_SIZE * 5; // += Texcoord, Normal
				numVertexElements += 2;

				if (mesh->HasTangentsAndBitangents())
				{
					Components.push_back({ &mesh->mTangents[0].x, 3 });
					Components.push_back({ &mesh->mBitangents[0].x, 3 });
					bTangents = true;
					vertexStrideSize += FLOAT_SIZE * 6; // += Tangents, Bitangents
					numVertexElements += 2;
				}
			}
			else
			{
				if (mesh->HasTangentsAndBitangents())
				{
					Components.push_back({ &mesh->mTextureCoords[0]->x, 2 });
					Components.push_back({ &mesh->mTangents[0].x, 3 });
					Components.push_back({ &mesh->mBitangents[0].x, 3 });
					bTexcoord = true;
					bTangents = true;
					vertexStrideSize += FLOAT_SIZE * 8; // += Texcoord, Tangents, Bitangents
					numVertexElements += 3;
				}
			}

			// float* pVertices = new float[mesh.mNumVertices * numVertexElements];
			float* pVertices = (float*)malloc(mesh->mNumVertices * vertexStrideSize);

			memset(pVertices, 0, mesh->mNumVertices * vertexStrideSize);

			const uint64_t maxSize = mesh->mNumVertices * numVertexElements * FLOAT_SIZE;

			{
				const unsigned int numVertices = mesh->mNumVertices;

				// Position   3f, bPosition
				// Texcoord   2f, bTexcoord
				// Normal     3f, bNormal
				// Tangents   3f, bTangents
				// biTangents 3f, bTangents

				size_t current{ 0 }; // In Vertices Index

				*/
/*
				// Version 1.
				for (size_t vertex{ 0 }; vertex < numVertices; ++vertex)
				{
					for (auto& component : Components)
					{
						memcpy_s(&pVertices[current], maxSize, component.first, component.second * FLOAT_SIZE); // 여기서 texcoord의 component.second는 2
						component.first = (component.first + component.second); // 여기서 texcoord의 component.second는 3
						current += component.second; // // 여기서 texcoord의 component.second는 2
					}
				}
				*/
				/*
				// Version 2.
for (size_t component{ 0 }; component < Components.size(); ++component)
{
	size_t jumpSize = (vertexStrideSize / FLOAT_SIZE);

	current = 0;

	for (size_t i{ 0 }; i < component; ++i)
	{
		current += Components[i].second;
	}

	if (component != 1)
	{
		for (size_t vertex{ 0 }; vertex < numVertices; ++vertex)
		{
			memcpy_s(&pVertices[current], maxSize, Components[component].first, Components[component].second * FLOAT_SIZE);
			Components[component].first = (Components[component].first + Components[component].second);
			current += jumpSize;
		}
	}
	else
	{
		for (size_t vertex{ 0 }; vertex < numVertices; ++vertex)
		{
			memcpy_s(&pVertices[current], maxSize, Components[component].first, Components[component].second * FLOAT_SIZE);
			Components[component].first = (Components[component].first + Components[component].second + 1);
			current += jumpSize;
		}
	}
}
			}

			delete[] pVertices;
			pVertices = nullptr;
		}
*/