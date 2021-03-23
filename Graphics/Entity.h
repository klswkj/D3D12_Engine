#pragma once
#include "Technique.h"
#include "UAVBuffer.h"
#include "Model.h"

class Material;
struct FundamentalVertexIndex;
struct aiMesh;

namespace custom
{
	class CommandContext;
}

class MasterRenderGraph;
class IWindow;
enum class eObjectFilterFlag;

class IEntity
{
	friend class Job;
public:
	IEntity() = default;
	virtual ~IEntity() = default;
	
	// IEntity(const Material& CMaterial, const aiMesh& _AiMesh, float _Scale = 1.0f) noexcept;
#if defined(_DEBUG)
	IEntity(const Material& CMaterial, FundamentalVertexIndex& Input, const float* pStartVertexLocation, std::string MeshName = {});
#else
	IEntity(const Material& CMaterial, FundamentalVertexIndex& Input, const float* pStartVertexLocation);
#endif
	IEntity(const IEntity&) = delete;
	void AddTechnique(Technique Tech) noexcept;
	virtual Math::Matrix4 GetTransform() const noexcept = 0;
	void Submit(eObjectFilterFlag Filter) noexcept;
	void Bind(custom::CommandContext& BaseContext, uint8_t commandIndex) const DEBUG_EXCEPT;
	void Accept(IWindow& window);
	void LinkTechniques(const MasterRenderGraph& _MasterRenderGraph);

	UINT GetIndexCount() const DEBUG_EXCEPT { return m_IndexCount; }
	UINT GetStartIndexLocation() const DEBUG_EXCEPT { return m_StartIndexLocation; }
	INT  GetBaseVertexLocation() const DEBUG_EXCEPT { return m_BaseVertexLocation; }

	struct BoundingBox
	{
		Math::Vector3 MinPoint = Math::Scalar(FLT_MAX);
		Math::Vector3 MaxPoint = Math::Scalar(FLT_MIN);
	};
	BoundingBox& GetBoundingBox() DEBUG_EXCEPT { return m_BoundingBox; }

	struct VertexLayout
	{
		float Position[3];
		float Normal[3];
		float TexCoord[2];
		float Tangent[3];
		float Bitangent[3];
		// float Padding[2];
	};

protected:
	void ComputeBoundingBox(const float* pVertices, size_t NumVertices, size_t VertexStride4ByteUnit = 0xe);

private:
	void LoadMesh(const aiMesh& _aiMesh, VertexLayout** pVertexLayout, uint16_t** pIndices);

protected:
#if defined(_DEBUG)
	std::wstring m_name;
#endif
	std::vector<Technique> m_Techniques;

	BoundingBox m_BoundingBox;

	D3D12_PRIMITIVE_TOPOLOGY m_Topology;

	UINT m_IndexCount         = -1; 
	UINT m_StartIndexLocation = -1;
	INT  m_BaseVertexLocation = -1;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView;
};