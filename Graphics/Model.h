#pragma once

// Model  ┌─ ModelPart* Root 
//        └─ vector<Mesh* : public IEntity>   ┌─ XMFLOAT4X4   m_Transform
//                                           │  VertexBuffer           <- class Material::Vertex (FLOAT3) =>  StructuredBuffer
//                                           │  IndexBuffer            <- class Material::Index  (FLOAT3) => ByteAddressBuffer
//                                           │  Topology               <  D3D11_PRIMITIVE_TOPOLOGY
//                                           └─ Technique  ┬─ vector<RenderStage>  ────────────┬─ vector<Bindable*>
//                                                         │  uint32_t Channel          │  ID3D12RenderQueuePass* ──────── vector<Job>
//                                                         └  bActive, string name      └  string targetPassName
// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//             ┌  IEntity*
// vector<Job> ┤  
//             └  RenderStage*

struct aiMesh;
struct aiMaterial;
struct aiNode;

class Mesh;
class ModelPart;
class ModelComponentWindow;
class MasterRenderGraph;

enum class eObjectFilterFlag;

// Head of ModelPart
class Model
{
    friend class ObjModelManager;
public:
	Model(const std::string& pathString, float scale = 1.0f);

	void Submit(eObjectFilterFlag Filter) const DEBUG_EXCEPT;
	void SetRootTransform(const Math::Matrix4 _Transfrom) noexcept;
	void TraverseNode(ModelComponentWindow& _ComponentWindow);
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph);
    void ComputeBoundingBox(); 
    struct MeshData
    {
        size_t VertexDataByteOffset;
        UINT VertexCount;
        // size_t VertexStride; // 이거 고정이잖아, Byte Size는 0x38로 그냥 Stride사이즈는 0xe로

        size_t IndexDataByteOffset;
        UINT IndexCount;
        // size_t IndexStride = sizeof(uint16_t);
    };
    struct BoundingBox
    {
        Math::Vector3 MinPoint = Math::Scalar(FLT_MAX);
        Math::Vector3 MaxPoint = Math::Scalar(FLT_MIN);
    };

    void LoadMesh(const aiScene* _aiScene, float** pVertices, uint16_t** pIndices, MeshData** pMeshDataArray, size_t VertexIndexCount[], float Scale = 1.0f);
    void OptimizeRemoveDuplicateVertices(MeshData* pMeshData, float* pVertices, uint16_t* pIndices, size_t NumVertices, size_t NumIndices, size_t& TotalVertexCount, size_t& TotalMeshCount);
    BoundingBox& GetBoundingBox() { return m_BoundingBox; }
    const std::string& GetName() { return m_ModelName; }
private:
	std::unique_ptr<ModelPart> ParseNode(uint32_t& NextId, const aiNode& _Node, float _Scale) noexcept;
    // std::unique_ptr<ModelPart> ParseNode(uint32_t& NextId, const aiNode& _Node, std::string NormalizedString, float _Scale);
private:
    std::string m_ModelName;
    std::vector<std::unique_ptr<Mesh>> m_pMeshes;
	std::unique_ptr<ModelPart> m_pModelPart;

    BoundingBox m_BoundingBox;
};