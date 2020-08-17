#include "VertexBuffer.h"

// ShaderInputLayout
const ShaderInputLayout::ShaderInput& ShaderInputLayout::ResolveByIndex(size_t i) const DEBUG_EXCEPT
{
	return shaderinputs[i];
}
ShaderInputLayout& ShaderInputLayout::Append(ShaderInputType type) DEBUG_EXCEPT
{
	if (!HasType(type))
	{
		shaderinputs.emplace_back(type, Size());
	}
	return *this;
}
bool ShaderInputLayout::HasType(ShaderInputType type) const noexcept
{
	for (auto& e : shaderinputs)
	{
		if (e.GetType() == type)
		{
			return true;
		}
	}
	return false;
}
size_t ShaderInputLayout::Size() const DEBUG_EXCEPT
{
	return (shaderinputs.empty()) ? 0u : shaderinputs.back().GetOffsetAfter();
}
size_t ShaderInputLayout::GetElementCount() const noexcept
{
	return shaderinputs.size();
}
std::vector<D3D12_INPUT_ELEMENT_DESC> ShaderInputLayout::GetD3DLayout() const DEBUG_EXCEPT
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> desc;
	desc.reserve(GetElementCount());

	for (const auto& e : shaderinputs)
	{
		desc.push_back(e.GetDesc());
	}
	return desc;
}
std::string ShaderInputLayout::GetCode() const DEBUG_EXCEPT
{
	std::string code;
	for (const auto& e : shaderinputs)
	{
		code += e.GetCode();
	}
	return code;
}


// ShaderInputLayout::ShaderInput
ShaderInputLayout::ShaderInput::ShaderInput(ShaderInputType type, size_t offset)
	: type(type), offset(offset)
{
}
size_t ShaderInputLayout::ShaderInput::GetOffsetAfter() const DEBUG_EXCEPT
{
	return offset + Size();
}
size_t ShaderInputLayout::ShaderInput::GetOffset() const
{
	return offset;
}
size_t ShaderInputLayout::ShaderInput::Size() const DEBUG_EXCEPT
{
	return SizeOf(type);
}
ShaderInputLayout::ShaderInputType ShaderInputLayout::ShaderInput::GetType() const noexcept
{
	return type;
}

template<ShaderInputLayout::ShaderInputType type>
struct SysSizeLookup
{
	static constexpr auto Exec() noexcept
	{
		return sizeof(ShaderInputLayout::Map<type>::SysType);
	}
};

constexpr size_t ShaderInputLayout::ShaderInput::SizeOf(ShaderInputType type) DEBUG_EXCEPT
{
	return Bridge<SysSizeLookup>(type);
}

template<ShaderInputLayout::ShaderInputType type>
struct CodeLookup
{
	static constexpr auto Exec() noexcept
	{
		return ShaderInputLayout::Map<type>::code;
	}
};


const char* ShaderInputLayout::ShaderInput::GetCode() const noexcept
{
	return Bridge<CodeLookup>(type);
}

template<ShaderInputLayout::ShaderInputType type> 
struct INPUT_ELEMENT
{
	static constexpr D3D12_INPUT_ELEMENT_DESC Exec(size_t offset) noexcept
	{
		return
		{
			ShaderInputLayout::Map<type>::semantic,    0, // SementicName, Semantic Index
			ShaderInputLayout::Map<type>::dxgiFormat,  0, // Format, inputSlot,
			(UINT)offset, D3D12_INPUT_PER_VERTEX_DATA, 0  // AlignedByteOffset, InputSlotclass 
		};
	}
};

D3D12_INPUT_ELEMENT_DESC ShaderInputLayout::ShaderInput::GetDesc() const DEBUG_EXCEPT
{
	return Bridge<INPUT_ELEMENT>(type, GetOffset());
}


// VertexData
VertexData::VertexData(char* pData, const ShaderInputLayout& layout) DEBUG_EXCEPT
	: pData(pData), layout(layout)
{
	ASSERT(pData != nullptr);
}

ConstVertex::ConstVertex(const VertexData& v) DEBUG_EXCEPT
	: vertex(v)
{
}


// VertexBuffer
VertexBuffer::VertexBuffer(ShaderInputLayout layout, size_t size) DEBUG_EXCEPT
	: layout(std::move(layout))
{
	Resize(size);
}
void VertexBuffer::Resize(size_t newSize) DEBUG_EXCEPT
{
	const auto size = Size();
	if (size < newSize)
	{
		buffer.resize(buffer.size() + layout.Size() * (newSize - size));
	}
}
const char* VertexBuffer::GetData() const DEBUG_EXCEPT
{
	return buffer.data();
}

template<ShaderInputLayout::ShaderInputType type>
struct AttributeAiMeshFill
{
	static constexpr void Extract(VertexBuffer* pBuf, const aiMesh& mesh) DEBUG_EXCEPT
	{
		for (auto end = mesh.mNumVertices, i = 0u; i < end; ++i)
		{
			(*pBuf)[i].Attr<type>() = ShaderInputLayout::Map<type>::Extract(mesh, i);
		}
	}
};

VertexBuffer::VertexBuffer(ShaderInputLayout layout_in, const aiMesh& mesh)
	: layout(std::move(layout_in))
{
	Resize(mesh.mNumVertices);
	for (size_t i = 0, end = layout.GetElementCount(); i < end; ++i)
	{
		ShaderInputLayout::Bridge<AttributeAiMeshFill>(layout.ResolveByIndex(i).GetType(), this, mesh);
	}
}

const ShaderInputLayout& VertexBuffer::GetLayout() const noexcept
{
	return layout;
}

size_t VertexBuffer::Size() const DEBUG_EXCEPT
{
	return buffer.size() / layout.Size();
}

size_t VertexBuffer::SizeBytes() const DEBUG_EXCEPT
{
	return buffer.size();
}

VertexData VertexBuffer::Back() DEBUG_EXCEPT
{
	ASSERT(buffer.size() != 0u);
	return VertexData{ buffer.data() + buffer.size() - layout.Size(),layout };
}

VertexData VertexBuffer::Front() DEBUG_EXCEPT
{
	ASSERT(buffer.size() != 0u);
	return VertexData{ buffer.data(),layout };
}

VertexData VertexBuffer::operator[](size_t i) DEBUG_EXCEPT
{
	ASSERT(i < Size());
	return VertexData{ buffer.data() + layout.Size() * i,layout };
}

ConstVertex VertexBuffer::Back() const DEBUG_EXCEPT
{
	return const_cast<VertexBuffer*>(this)->Back();
}

ConstVertex VertexBuffer::Front() const DEBUG_EXCEPT
{
	return const_cast<VertexBuffer*>(this)->Front();
}

ConstVertex VertexBuffer::operator[](size_t i) const DEBUG_EXCEPT
{
	return const_cast<VertexBuffer&>(*this)[i];
}