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
	
	Entity(const Material& CMaterial, const aiMesh& _AiMesh, float _Scale = 1.0f) noexcept;
	Entity(const Entity&) = delete;
	void AddTechnique(Technique Tech) noexcept;
	virtual DirectX::XMMATRIX GetTransformXM() const noexcept = 0;
	void Submit(eObjectFilterFlag Filter) const noexcept;
	void Bind(custom::CommandContext& BaseContext) const DEBUG_EXCEPT;
	void Accept(ITechniqueWindow& window);
	uint32_t GetIndexCount() const DEBUG_EXCEPT;
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph);

	struct BoundingBox
	{
		Math::Vector3 Min;
		Math::Vector3 Max;
	};
	struct VertexLayout
	{
		float Position[3];
		float Normal[3];
		float TexCoord[2];
		float Tangent[3];
		float Bitangent[3];
		// float Padding[2];
	};

private:
	void LoadMesh(const aiMesh& _aiMesh, VertexLayout** pVertexLayout, uint16_t** pIndices);
	void ComputeBoundingBox(float* pVertices, size_t NumVertices);

protected:
	std::vector<Technique> techniques;

	BoundingBox m_BoundingBox;
	custom::StructuredBuffer  m_VerticesBuffer; // ���� ���� �ڸ��� ����  
	custom::ByteAddressBuffer m_IndicesBuffer;  // ""
	D3D12_PRIMITIVE_TOPOLOGY m_Topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };

	UINT m_IndexCount = -1; 
	UINT m_StartIndexLocation = -1;
	INT m_BaseVertexLocation = -1;

	/*
	custom::StructuredBuffer* m_pVerticesBuffer;
	custom::ByteAddressBuffer* m_pIndicesBuffer;
	
	size_t m_VertexStride;         -> ��
	size_t m_VertexDataByteOffset; -> ��

	size_t m_IndexDataByteOffset;  -> ��
	size_t m_IndexCount;           -> ��
	*/
	// �׷� ������ VertexBuffer��, IndexBuffer�� 
	// ���� �����س��� �����͸� �������� �ؾߵ�.
	// std::map<size_t, shared_ptr<StructedBuffer>> s_VertexContainer;
	// std::map<size_t, shared_ptr<ByteAddressBuffer>> s_ByteAddressBuffer; �� ���� �ϴ°� �� ������...
	// �ٵ�.. �� ���� �ΰ� StructedBuffer, ByteAddressBuffer�� ���ص� ���ݾ�...?
};


/*
struct Material
{
	Vector3 diffuse;
	Vector3 specular;
	Vector3 ambient;
	Vector3 emissive;
	Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
	float opacity;
	float shininess; // specular exponent
	float specularStrength; // multiplier on top of specular color

	enum { maxTexPath = 128 };
	enum { texCount = 6 };
	char texDiffusePath[maxTexPath];
	char texSpecularPath[maxTexPath];
	char texEmissivePath[maxTexPath];
	char texNormalPath[maxTexPath];
	char texLightmapPath[maxTexPath];
	char texReflectionPath[maxTexPath];

	enum { maxMaterialName = 128 };
	char name[maxMaterialName];
};





*/