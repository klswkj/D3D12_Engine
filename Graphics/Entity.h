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

class Entity
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

	custom::StructuredBuffer  m_VerticesBuffer;   
	custom::ByteAddressBuffer m_IndicesBuffer;
	D3D12_PRIMITIVE_TOPOLOGY m_Topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
};