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
class ITechniqueWindow;

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
	void Accept(ITechniqueWindow& window);
	uint32_t GetIndexCount() const DEBUG_EXCEPT;
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph);
protected:
	std::vector<Technique> techniques;

	// 여기 Index, Vertex, Toplogy를 원래 쓰던거로 변경하기.
	custom::StructuredBuffer  m_VerticesBuffer;   
	custom::ByteAddressBuffer m_IndicesBuffer;
	D3D12_PRIMITIVE_TOPOLOGY m_Topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
};
/*
enum D3D12_PRIMITIVE_TOPOLOGY_TYPE
	{
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED	= 0,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT	= 1,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE	= 2,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH	= 4
	} 	D3D12_PRIMITIVE_TOPOLOGY_TYPE;
*/