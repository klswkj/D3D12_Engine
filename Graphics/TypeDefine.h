#pragma once
#include <stdint.h>

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

typedef uint32_t GPUHeapIndex;
typedef uint32_t TextureIndex;
typedef uint32_t PipelineStateIndex;
typedef uint32_t RootSignatureIndex;
typedef uint32_t ResourceIndex;

typedef uint32_t RenderTargetIndex;
typedef uint32_t DepthStencilIndex;

// Engine Constants

constexpr uint32_t kFrameBufferCount = 3; // Triple buffered
constexpr uint32_t kFrameMaxDescriptorHeapCount = 2048; // Number of descriptors in the main GPU facing descriptor heap
constexpr uint32_t kMaxTextureCount = 512; // Max number of textures
constexpr uint32_t kMaxConstantBufferCount = 512; // Max count of constant buffers
constexpr uint32_t kMaxConstantBufferSize = _KB(256); //256KB

constexpr uint32_t kMaxD3DResources = 1024; // Max D3D12 resources.
constexpr uint32_t kMaxPipelineStates = 96;
constexpr uint32_t kMaxRootSignatures = 24;
constexpr uint32_t kRootParentEntityIndex = -1; // Default Root parent index. 

constexpr uint32_t kMaxRenderTargets = 64;
constexpr uint32_t kMaxDepthStencils = 16;

// Engine Types

struct ConstantBufferView
{
	uint64_t Offset;
	uint32_t GPUHeapIndex;
};

struct DataPack
{
	void*    ptr;
	uint32_t size;
};

// Offset of resources in GPU Descriptor heap 
struct GPUHeapOffsets
{
	uint64_t constantBufferOffset;
	uint64_t texturesOffset;
	uint64_t materialsOffset;
};

enum class TextureType
{
	WIC,
	DDS
};

enum class RootParameterSlot
{
	RootSigCBVertex0,
	RootSigCBPixel0,
	RootSigSRVPixel1,
	RootSigSRVPixel2,
	RootSigCBAll1,
	RootSigCBAll2,
	RootSigIBL,
	RootSigUAV0,
	// RootSigParamCount
};

struct ScreenSize
{
	int Width;
	int Height;
};

struct SceneRenderTarget
{
	uint32_t TextureIndex;
	uint32_t ResourceInex;
	uint32_t RenderTargetIndex;
};

struct DepthTarget
{
	uint32_t TextureIndex;
	uint32_t ResourceInex;
	uint32_t RenderTargetIndex;
};

struct ColorValues
{
	static constexpr float ClearColor[] = { 0, 0, 0, 1 };
};