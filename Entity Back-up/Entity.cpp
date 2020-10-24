#include "stdafx.h"
#include "Entity.h"
#include "Mesh.h"
#include "Material.h"
#include "CommandContext.h"
#include "TechniqueWindow.h"
#include <assimp/scene.h>
#include <assimp/mesh.h>

Entity::Entity(const Material& mat, const aiMesh& mesh, float scale/* = 1.0f*/) noexcept
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

		if (mesh.HasPositions())
		{
			VertexElements.push_back({ &mesh.mVertices[0].x, 3 });
			bPosition = true;
			vertexStrideSize += FLOAT_SIZE * 3; // += POSITION
			++numVertexElements;
		}
		if (mesh.HasNormals())
		{
			VertexElements.push_back({ &mesh.mNormals[0].x, 3 });
			bNormal = true;
			
			vertexStrideSize += FLOAT_SIZE * 3; // += Texcoord, Normal
			++numVertexElements;
		}
		if (mesh.HasTextureCoords(0))
		{
			VertexElements.push_back({ &mesh.mTextureCoords[0]->x, 2 });
			bTexcoord = true;
			vertexStrideSize += FLOAT_SIZE * 2; // += Texcoord, Normal
			++numVertexElements;
		}
		if (mesh.HasTangentsAndBitangents())
		{
			VertexElements.push_back({ &mesh.mTangents[0].x, 3 });
			VertexElements.push_back({ &mesh.mBitangents[0].x, 3 });
			bTexcoord = true;
			bTangents = true;
			vertexStrideSize += FLOAT_SIZE * 6; // += Texcoord, Tangents, Bitangents
			numVertexElements += 3;
		}
		
		float* pVertices = (float*)malloc(mesh.mNumVertices * vertexStrideSize);
		memset(pVertices, 0, mesh.mNumVertices * vertexStrideSize); // Zeromemory(pVertices[0], mesh.mNumVertices * vertexStrideSize

		const uint64_t maxSize = mesh.mNumVertices * numVertexElements * FLOAT_SIZE;

		{
			size_t numVertices = mesh.mNumVertices;

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

					if (IVertexElement == 0 && scale != 1.0f)
					{
						for (size_t IVertex = 0; IVertex < numVertices; ++IVertex)
						{
							pVertices[jumpSize * IVertex] *= scale;
							pVertices[jumpSize * IVertex + 1] *= scale;
							pVertices[jumpSize * IVertex + 2] *= scale;
						}
					}
				}
			}
		}

		m_VerticesBuffer.Create(AnsiToWString(mesh.mName.C_Str()) + L" Vertex Buffer", mesh.mNumVertices, vertexStrideSize, pVertices);

		delete[] pVertices;
		pVertices = nullptr;
	} // End VertexBuffer

	if (mesh.HasFaces())
	{
		m_IndicesBuffer.Create(AnsiToWString(mesh.mName.C_Str()) + L" Index Buffer", mesh.mNumFaces, sizeof(unsigned short) * 3, (void*)mesh.mFaces);
	}

	for (auto& Tech : mat.GetTechniques())
	{
		AddTechnique(std::move(Tech));
	}
}

void Entity::AddTechnique(Technique _Technique) noexcept
{
	_Technique.InitializeParentReferences(*this);
	techniques.push_back(std::move(_Technique));
}
void Entity::Submit(eObjectFilterFlag Filter) const noexcept
{
	for (const std::vector<Technique>::iterator::value_type& Tech : techniques)
	{
		Tech.Submit(*this, Filter);
	}
}
void Entity::Bind(custom::CommandContext& BaseContext) const DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetVertexBuffer(0, m_VerticesBuffer.VertexBufferView());
	graphicsContext.SetIndexBuffer(m_IndicesBuffer.IndexBufferView());
	graphicsContext.SetPrimitiveTopology(m_Topology);
}
void Entity::Accept(ITechniqueWindow& window)
{
	for (auto& Tech : techniques)
	{
		Tech.Accept(window); // For Outline Technique
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