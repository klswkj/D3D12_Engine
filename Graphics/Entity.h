#pragma once
#include "Technique.h"
#include "UAVBuffer.h"
class Material;
struct aiMesh;

namespace custom
{
	class CommandContext;
}

class MasterRenderGraph;
class AddonWindow;

enum class eObjectFilterFlag;

class Entity // Drawble
{
public:
	Entity() = default;
	virtual ~Entity() {};
	
	Entity(const Material& mat, const aiMesh& mesh, float scale = 1.0f) noexcept;
	Entity(const Entity&) = delete;
	void AddTechnique(Technique Tech) noexcept;
	virtual DirectX::XMMATRIX GetTransformXM() const noexcept = 0;
	void Submit(eObjectFilterFlag Filter) const noexcept;
	void Bind(custom::CommandContext& BaseContext) const DEBUG_EXCEPT;
	void Accept(AddonWindow& window);
	uint32_t GetIndexCount() const DEBUG_EXCEPT;
	void LinkTechniques(MasterRenderGraph&);
protected:
	std::vector<Technique> techniques;

	// 여기 Index, Vertex, Toplogy를 원래 쓰던거로 변경하기.
	custom::StructuredBuffer  m_VerticesBuffer;   
	custom::ByteAddressBuffer m_IndicesBuffer;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_Topology{D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE};
};

// 결국은 VertexBuffer, IndexBuffer ID3D12Resource랑, ResourceView(DescriptorHandle)로 변경해야됨.
