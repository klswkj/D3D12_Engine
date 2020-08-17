#pragma once
#include "stdafx.h"
#include <assimp/scene.h>

#include "Color.h"

#define DVTX_ELEMENT_AI_EXTRACTOR(member)                      \
static SysType Extract(const aiMesh& mesh, size_t i) noexcept  \
{                                                              \
	return *reinterpret_cast<const SysType*>(&mesh.member[i]); \
}

#define SHADER_INPUT_TYPES \
	X( Position2D ) \
	X( Position3D ) \
	X( Texture2D ) \
	X( Normal ) \
	X( Tangent ) \
	X( Bitangent ) \
	X( Float3Color ) \
	X( Float4Color ) \
	X( BGRAColor ) \
	X( Count )

// [Code Navigator]
// class ShaderInputLayout
// class ShaderInput
// class VertexData
// class ConstVertex
// class VertexBuffer

class ShaderInputLayout
{
public:
	enum ShaderInputType
	{
#define X(el) el,
		SHADER_INPUT_TYPES
#undef X
	};

	template<ShaderInputType>
	struct Map;

	template<>
	struct Map<Position2D>
	{
		using SysType = DirectX::XMFLOAT2;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
		static constexpr const char* semantic = "Position";
		static constexpr const char* code = "P2";
		DVTX_ELEMENT_AI_EXTRACTOR(mVertices)
	};

	template<>
	struct Map<Position3D>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Position";
		static constexpr const char* code = "P3";
		DVTX_ELEMENT_AI_EXTRACTOR(mVertices)
	};

	template<>
	struct Map<Texture2D>
	{
		using SysType = DirectX::XMFLOAT2;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
		static constexpr const char* semantic = "Texcoord";
		static constexpr const char* code = "T2";
		DVTX_ELEMENT_AI_EXTRACTOR(mTextureCoords[0])
	};

	template<>
	struct Map<Normal>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Normal";
		static constexpr const char* code = "N";
		DVTX_ELEMENT_AI_EXTRACTOR(mNormals)
	};

	template<>
	struct Map<Tangent>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Tangent";
		static constexpr const char* code = "Nt";
		DVTX_ELEMENT_AI_EXTRACTOR(mTangents)
	};

	template<>
	struct Map<Bitangent>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Bitangent";
		static constexpr const char* code = "Nb";
		DVTX_ELEMENT_AI_EXTRACTOR(mBitangents)
	};

	template<>
	struct Map<Float3Color>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		static constexpr const char* semantic = "Color";
		static constexpr const char* code = "C3";
		DVTX_ELEMENT_AI_EXTRACTOR(mColors[0])
	};

	template<>
	struct Map<Float4Color>
	{
		using SysType = DirectX::XMFLOAT4;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		static constexpr const char* semantic = "Color";
		static constexpr const char* code = "C4";
		DVTX_ELEMENT_AI_EXTRACTOR(mColors[0])
	};

	template<>
	struct Map<BGRAColor>
	{
		using SysType = custom::Color;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		static constexpr const char* semantic = "Color";
		static constexpr const char* code = "C8";
		DVTX_ELEMENT_AI_EXTRACTOR(mColors[0])
	};

	template<>
	struct Map<Count>
	{
		using SysType = long double;
		static constexpr DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
		static constexpr const char* semantic = "!INVALID!";
		static constexpr const char* code = "!INV!";
		DVTX_ELEMENT_AI_EXTRACTOR(mFaces)
	};

	template<template<ShaderInputLayout::ShaderInputType> class F, typename... Args>
	static constexpr auto Bridge(ShaderInputLayout::ShaderInputType type, Args&&... args) DEBUG_EXCEPT
	{
		switch (type)
		{
#define X(el)                                                                       \
			case (ShaderInputLayout::el):                                           \
			{                                                                       \
				return F<ShaderInputLayout::el>::Exec(std::forward<Args>(args)...); \
			}
			SHADER_INPUT_TYPES
#undef X                                                                           
		}

		ASSERT(false, "Invalid element type");
		return F<ShaderInputLayout::Count>::Exec(std::forward<Args>(args)...);
	}

	// Class ShaderInput
	// ShaderInputType type, size_t offset
	class ShaderInput
	{
	public:
		ShaderInput(ShaderInputType type, size_t offset);
		size_t GetOffsetAfter() const DEBUG_EXCEPT;
		size_t GetOffset() const;
		size_t Size() const DEBUG_EXCEPT;
		static constexpr size_t SizeOf(ShaderInputType type) DEBUG_EXCEPT;
		ShaderInputType GetType() const noexcept;
		D3D12_INPUT_ELEMENT_DESC GetDesc() const DEBUG_EXCEPT;
		const char* GetCode() const noexcept;
	private:
		ShaderInputType type;
		size_t offset;
	};

public:
	template<ShaderInputType Type>
	const ShaderInput& Resolve() const DEBUG_EXCEPT
	{
		for (auto& element : shaderinputs)
		{
			if (element.GetType() == Type)
			{
				return element;
			}
		}
		ASSERT(false, "Could not resolve element type");
		return shaderinputs.front();
	}

	const ShaderInput& ResolveByIndex(size_t i) const DEBUG_EXCEPT;
	ShaderInputLayout& Append(ShaderInputType type) DEBUG_EXCEPT;
	size_t Size() const DEBUG_EXCEPT;
	size_t GetElementCount() const noexcept;
	std::vector<D3D12_INPUT_ELEMENT_DESC> GetD3DLayout() const DEBUG_EXCEPT;
	std::string GetCode() const DEBUG_EXCEPT;
	bool HasType(ShaderInputType type) const noexcept;

private:
	std::vector<ShaderInput> shaderinputs;
}; // class ShaderInputLayout End


class VertexData
{
	friend class VertexBuffer;
private:
	// necessary for Bridge to SetAttribute
	template<ShaderInputLayout::ShaderInputType type>
	struct AttributeSetting
	{
		template<typename T>
		static constexpr auto Exec(VertexData* pVertex, char* pAttribute, T&& val) DEBUG_EXCEPT
		{
			return pVertex->SetAttribute<type>(pAttribute, std::forward<T>(val));
		}
	};
public:
	template<ShaderInputLayout::ShaderInputType Type>
	auto& Attr() DEBUG_EXCEPT
	{
		auto pAttribute = pData + layout.Resolve<Type>().GetOffset();
		return *reinterpret_cast<typename ShaderInputLayout::Map<Type>::SysType*>(pAttribute);
	}
	template<typename T>
	void SetAttributeByIndex(size_t i, T&& val) DEBUG_EXCEPT
	{
		const auto& element = layout.ResolveByIndex(i);
		auto pAttribute = pData + element.GetOffset();
		ShaderInputLayout::Bridge<AttributeSetting>
			(
				element.GetType(), this, pAttribute, std::forward<T>(val)
				);
	}

protected:
	VertexData(char* pData, const ShaderInputLayout& layout) DEBUG_EXCEPT;

private:
	// enables parameter pack setting of multiple parameters by element index
	template<typename First, typename ...Rest>
	void SetAttributeByIndex(size_t i, First&& first, Rest&&... rest) DEBUG_EXCEPT
	{
		SetAttributeByIndex(i, std::forward<First>(first));
		SetAttributeByIndex(i + 1, std::forward<Rest>(rest)...);
	}

	// helper to reduce code duplication in SetAttributeByIndex
	template<ShaderInputLayout::ShaderInputType DestLayoutType, typename SrcType>
	void SetAttribute(char* pAttribute, SrcType&& val) DEBUG_EXCEPT
	{
		using Dest = typename ShaderInputLayout::Map<DestLayoutType>::SysType;

		if constexpr (std::is_assignable<Dest, SrcType>::value)
		{
			*reinterpret_cast<Dest*>(pAttribute) = val;
		}
		else
		{
			ASSERT(false, "Parameter attribute type mismatch");
		}
	}
private:
	char* pData = nullptr;
	const ShaderInputLayout& layout;
};


class ConstVertex
{
public:
	ConstVertex(const VertexData& v) DEBUG_EXCEPT;
	template<ShaderInputLayout::ShaderInputType Type>
	const auto& Attr() const DEBUG_EXCEPT
	{
		return const_cast<VertexData&>(vertex).Attr<Type>();
	}
private:
	VertexData vertex;
};


class VertexBuffer
{
public:
	VertexBuffer(ShaderInputLayout layout, size_t size = 0u) DEBUG_EXCEPT;
	VertexBuffer(ShaderInputLayout layout, const aiMesh& mesh);
	const char* GetData() const DEBUG_EXCEPT;
	const ShaderInputLayout& GetLayout() const noexcept;
	void Resize(size_t newSize) DEBUG_EXCEPT;
	size_t Size() const DEBUG_EXCEPT;
	size_t SizeBytes() const DEBUG_EXCEPT;
	template<typename ...Params>
	void EmplaceBack(Params&&... params) DEBUG_EXCEPT
	{
		ASSERT(sizeof...(params) == layout.GetElementCount(), "Param count doesn't match number of vertex shaderinputs");
		buffer.resize(buffer.size() + layout.Size());
		Back().SetAttributeByIndex(0u, std::forward<Params>(params)...);
	}
	VertexData Back() DEBUG_EXCEPT;
	VertexData Front() DEBUG_EXCEPT;
	VertexData operator[](size_t i) DEBUG_EXCEPT;
	ConstVertex Back() const DEBUG_EXCEPT;
	ConstVertex Front() const DEBUG_EXCEPT;
	ConstVertex operator[](size_t i) const DEBUG_EXCEPT;
private:
	std::vector<char> buffer;
	ShaderInputLayout layout;
};


#undef DVTX_ELEMENT_AI_EXTRACTOR
#ifndef DVTX_SOURCE_FILE
#undef SHADER_INPUT_TYPES
#endif