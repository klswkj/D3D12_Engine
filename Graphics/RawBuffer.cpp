#include "stdafx.h"
#include "RawBuffer.h"
#include "LayoutCodex.h"

RawBuffer::RawBuffer(RawLayout&& LayOut) DEBUG_EXCEPT
	: RawBuffer(LayoutCodex::Resolve(std::move(LayOut)))
{
}
RawBuffer::RawBuffer(const ManagedLayout& Layout) DEBUG_EXCEPT
	: pData(Layout.ShareRoot()), bytes(pData->GetOffsetEnd())
{
}
RawBuffer::RawBuffer(ManagedLayout&& Layout) DEBUG_EXCEPT
	: pData(Layout.attachData()), bytes(pData->GetOffsetEnd())
{
}
RawBuffer::RawBuffer(const RawBuffer& Buffer) noexcept
	: pData(Buffer.pData), bytes(Buffer.bytes)
{
}
RawBuffer::RawBuffer(RawBuffer&& Buffer) noexcept
	: pData(std::move(Buffer.pData)), bytes(std::move(Buffer.bytes))
{
}
void RawBuffer::CopyFrom(const RawBuffer& other) DEBUG_EXCEPT
{
	ASSERT(&GetRootLayoutElement() == &other.GetRootLayoutElement());
	// std::copy(other.bytes.begin(), other.bytes.end(), bytes.begin());
	SIMDMemCopy((void*)(bytes.data()), (void*)(other.bytes.data()), HashInternal::DivideByMultiple(other.bytes.size() * sizeof(char), 16));

}

const char* RawBuffer::GetData() const noexcept
{
	return bytes.data();
}
size_t RawBuffer::GetSizeInBytes() const noexcept
{
	return bytes.size();
}
const IPolymorphData& RawBuffer::GetRootLayoutElement() const noexcept
{
	return *pData;
}
std::shared_ptr<IPolymorphData> RawBuffer::ShareLayoutRoot() const noexcept
{
	return pData;
}

// Actual implementation of ImplBase
struct ExtraData
{
	struct Struct : public IPolymorphData::ImplBase
	{
		std::vector<std::pair<std::string, IPolymorphData>> layoutElements;
	};
	struct Array : public IPolymorphData::ImplBase
	{
		std::optional<IPolymorphData> layoutElement;
		size_t element_size;
		size_t size;
	};
};

std::string IPolymorphData::GetSignature() const DEBUG_EXCEPT
{
	using namespace custom;

	switch (type)
	{
#define X(el) case Type::el : return Map<Type::el>::code;
		PRIMITIVE_DATA_TYPES
#undef X
	case custom::Type::Struct:
		return getSignatureForStruct();
	case custom::Type::Array:
		return getSignatureForArray();
	default:
		ASSERT(0, "Bad type in signature generation");
		return "";
	}
}
bool IPolymorphData::Exists() const noexcept
{
	return type != custom::Type::Empty;
}
std::pair<size_t, const IPolymorphData*> IPolymorphData::CalculateIndexingOffset(size_t offset, size_t index) const DEBUG_EXCEPT
{
	ASSERT(type == custom::Type::Array);
	const auto& data = static_cast<ExtraData::Array&>(*pImplData);
	ASSERT(index < data.size);
	return { offset + data.element_size * index, &(*(data.layoutElement)) };
}
IPolymorphData& IPolymorphData::operator[](const std::string& Key) DEBUG_EXCEPT
{
	ASSERT(type == custom::Type::Struct);

	for (auto& mem : static_cast<ExtraData::Struct&>(*pImplData).layoutElements)
	{
		if (mem.first == Key)
		{
			return mem.second;
		}
	}

	static IPolymorphData empty{};
	return empty;
}
const IPolymorphData& IPolymorphData::operator[](const std::string& Key) const DEBUG_EXCEPT
{
	return const_cast<IPolymorphData&>(*this)[Key];
}
IPolymorphData& IPolymorphData::T() DEBUG_EXCEPT
{
	ASSERT(type == custom::Type::Array);
	return *static_cast<ExtraData::Array&>(*pImplData).layoutElement;
}
const IPolymorphData& IPolymorphData::T() const DEBUG_EXCEPT
{
	return const_cast<IPolymorphData&>(*this).T();
}
size_t IPolymorphData::GetOffsetBegin() const DEBUG_EXCEPT
{
	return *offset;
}
size_t IPolymorphData::GetOffsetEnd() const DEBUG_EXCEPT
{
	using namespace custom;

	switch (type)
	{
#define X(el) case Type::el :  return *offset + Map<Type::el>::hlslSize; 
		PRIMITIVE_DATA_TYPES
#undef X
	case custom::Type::Struct:
		{
			const auto& data = static_cast<ExtraData::Struct&>(*pImplData);
			return AdvanceToBoundary(data.layoutElements.back().second.GetOffsetEnd());
		}
	case custom::Type::Array:
	{
		const auto& data = static_cast<ExtraData::Array&>(*pImplData);
		return *offset + AdvanceToBoundary(data.layoutElement->GetSizeInBytes()) * data.size;
	}
	default:
		ASSERT(0, "Invalid element");
		return 0u;
	}
}
size_t IPolymorphData::GetSizeInBytes() const DEBUG_EXCEPT
{
	return GetOffsetEnd() - GetOffsetBegin();
}
IPolymorphData& IPolymorphData::Add(custom::Type Type, std::string Key) DEBUG_EXCEPT
{
	ASSERT(type == custom::Type::Struct, "Add to non-struct in layout");
	ASSERT(ValidateSymbolName(Key), "invalid symbol m_Name in Struct");

	auto& structData = static_cast<ExtraData::Struct&>(*pImplData);
	for (auto& mem : structData.layoutElements)
	{
		if (mem.first == Key)
		{
			ASSERT(0, "Adding duplicate m_Name to struct");
		}
	}
	structData.layoutElements.emplace_back(std::move(Key), IPolymorphData{ Type });
	return *this;
}
IPolymorphData& IPolymorphData::Set(custom::Type Type, size_t Size) DEBUG_EXCEPT
{
	ASSERT(type == custom::Type::Array);
	ASSERT(Size != 0u);

	auto& arrayData = static_cast<ExtraData::Array&>(*pImplData);
	arrayData.layoutElement = { Type };
	arrayData.size = Size;
	return *this;
}
IPolymorphData::IPolymorphData(custom::Type Type) DEBUG_EXCEPT
	: type{ Type }
{
	ASSERT(Type != custom::Type::Empty);
	if (Type == custom::Type::Struct)
	{
		pImplData = std::unique_ptr<ExtraData::Struct>{ new ExtraData::Struct() };
	}
	else if (Type == custom::Type::Array)
	{
		pImplData = std::unique_ptr<ExtraData::Array>{ new ExtraData::Array() };
	}
}
size_t IPolymorphData::finalize(size_t OffSet) DEBUG_EXCEPT
{
	using namespace custom;

	switch (type)
	{
#define X(el) case Type::el : offset = AdvanceIfCrossesBoundary(OffSet, Map<Type::el>::hlslSize); return *offset + Map<Type::el>::hlslSize;
		PRIMITIVE_DATA_TYPES
#undef X
	case Type::Struct:
		return finalizeForStruct(OffSet);
	case Type::Array:
		return finalizeForArray(OffSet);
	default:
		ASSERT(0, "Invalid type.");
		return 0;
	}
}
std::string IPolymorphData::getSignatureForStruct() const DEBUG_EXCEPT
{
	using namespace std::string_literals;
	auto sig = "St{"s;
	for (const auto& el : static_cast<ExtraData::Struct&>(*pImplData).layoutElements)
	{
		sig += el.first + ":"s + el.second.GetSignature() + ";"s;
	}
	sig += "}"s;
	return sig;
}
std::string IPolymorphData::getSignatureForArray() const DEBUG_EXCEPT
{
	using namespace std::string_literals;
	const auto& data = static_cast<ExtraData::Array&>(*pImplData);
	return "Ar:"s + std::to_string(data.size) + "{"s + data.layoutElement->GetSignature() + "}"s;
}
size_t IPolymorphData::finalizeForStruct(size_t OffSet)
{
	auto& data = static_cast<ExtraData::Struct&>(*pImplData);
	ASSERT(data.layoutElements.size() != 0u);
	offset = AdvanceToBoundary(OffSet);
	auto offsetNext = *offset;
	for (auto& el : data.layoutElements)
	{
		offsetNext = el.second.finalize(offsetNext);
	}
	return offsetNext;
}
size_t IPolymorphData::finalizeForArray(size_t OffSet)
{
	auto& data = static_cast<ExtraData::Array&>(*pImplData);
	ASSERT(data.size != 0u);
	offset = AdvanceToBoundary(OffSet);
	data.layoutElement->finalize(*offset);
	data.element_size = IPolymorphData::AdvanceToBoundary(data.layoutElement->GetSizeInBytes());
	return GetOffsetEnd();
}
bool IPolymorphData::CrossesBoundary(size_t Offset, size_t Size) noexcept
{
	const auto end = Offset + Size;
	const auto pageStart = Offset / 16u;
	const auto pageEnd = end / 16u;
	return (pageStart != pageEnd && end % 16 != 0u) || Size > 16u;
}
size_t IPolymorphData::AdvanceIfCrossesBoundary(size_t offset, size_t size) noexcept
{
	return CrossesBoundary(offset, size) ? AdvanceToBoundary(offset) : offset;
}
size_t IPolymorphData::AdvanceToBoundary(size_t offset) noexcept
{
	return offset + (16u - offset % 16u) % 16u;
}
bool IPolymorphData::ValidateSymbolName(const std::string& name) noexcept
{
	// symbols can contain alphanumeric and underscore, must not start with digit
	return !name.empty() &&
		!std::isdigit(name.front()) &&
		std::all_of(name.begin(), name.end(), [](char c)
			{
				return std::isalnum(c) || c == '_';
			}
	);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

Layout::Layout(std::shared_ptr<IPolymorphData> pPolymorphicData) noexcept
	: pPolymorphicData{ std::move(pPolymorphicData) }
{
}
size_t Layout::GetSizeInBytes() const noexcept
{
	return pPolymorphicData->GetSizeInBytes();
}
std::string Layout::GetSignature() const DEBUG_EXCEPT
{
	return pPolymorphicData->GetSignature();
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

RawLayout::RawLayout() noexcept
	: Layout{ std::shared_ptr<IPolymorphData>{ new IPolymorphData(custom::Type::Struct) } }
{
}
IPolymorphData& RawLayout::operator[](const std::string& key) DEBUG_EXCEPT
{
	return (*pPolymorphicData)[key];
}
void RawLayout::clearData() noexcept
{
	*this = RawLayout();
}
std::shared_ptr<IPolymorphData> RawLayout::moveData() noexcept
{
	auto temp = std::move(pPolymorphicData);
	temp->finalize(0);
	*this = RawLayout();
	return std::move(temp);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ManagedLayout::ManagedLayout(std::shared_ptr<IPolymorphData> pPolymorphicData) noexcept
	: Layout(std::move(pPolymorphicData))
{
}
const IPolymorphData& ManagedLayout::operator[](const std::string& key) const DEBUG_EXCEPT
{
	return (*pPolymorphicData)[key];
}
std::shared_ptr<IPolymorphData> ManagedLayout::ShareRoot() const noexcept
{
	return pPolymorphicData;
}
std::shared_ptr<IPolymorphData> ManagedLayout::attachData() const noexcept
{
	return std::move(pPolymorphicData);
}
