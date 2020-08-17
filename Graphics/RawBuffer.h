#pragma once
#include <vector>
#include <optional>
#include <string>
#include <memory>

#include <DirectXMath.h>

#ifndef DEBUG_EXCEPT
#define DEBUG_EXCEPT noexcept(!_DEBUG)
#endif

#include "CustomAssert.h"

//                                                                                       
//                  Layout式式式式式式式式式式式式式式成式 IPolymorphData 式成式 ExtraData 式式式式式式式式 Struct or Array
//                                      弛                  弛  DataType  式式式式式式式式 Float, Float2, Float3, Float4, Matrix4x4, Bool Integer32               
//                    弛                                    戌  Offset
//            忙式式式式式式式扛式式式式式式忖
//      RawLayout       ManagedLayout
//
//
//
//     
//

// class IPolymorphData
// class Layout
// class ManagedLayout
// class RawBuffer

//      RawBuffer["Scale"]
//      RawBuffer["Color"]
//      RawBuffer["specularIntensity"]
//      RawBuffer["specularPower"] 

#define PRIMITIVE_DATA_TYPES \
	X( Float )               \
	X( Float2 )              \
	X( Float3 )              \
	X( Float4 )              \
	X( Matrix )              \
	X( Bool )                \
	X( Integer )

namespace custom
{
	enum class Type
	{
#define X(el) el,
		PRIMITIVE_DATA_TYPES
#undef X
		Struct,
		Array,
		Empty,
	};

	template<Type T>
	struct Map
	{
	};

	template<>
	struct Map<Type::Float>
	{
		using SysType = float;
		static constexpr size_t hlslSize = sizeof(float);
		static constexpr const char* code = "F1";
	};

	template<>
	struct Map<Type::Float2>
	{
		using SysType = DirectX::XMFLOAT2;
		static constexpr size_t hlslSize = sizeof(DirectX::XMFLOAT2);
		static constexpr const char* code = "F2";
	};

	template<>
	struct Map<Type::Float3>
	{
		using SysType = DirectX::XMFLOAT3;
		static constexpr size_t hlslSize = sizeof(DirectX::XMFLOAT3);
		static constexpr const char* code = "F3";
	};

	template<>
	struct Map<Type::Float4>
	{
		using SysType = DirectX::XMFLOAT4;
		static constexpr size_t hlslSize = sizeof(DirectX::XMFLOAT4);
		static constexpr const char* code = "F4";
	};
	template<>
	struct Map<Type::Matrix>
	{
		using SysType = DirectX::XMFLOAT4X4;
		static constexpr size_t hlslSize = sizeof(DirectX::XMFLOAT4X4);
		static constexpr const char* code = "M4";
	};

	template<>
	struct Map<Type::Bool>
	{
		using SysType = bool;
		static constexpr size_t hlslSize = 4u;
		static constexpr const char* code = "BL";
	};

	template<>
	struct Map<Type::Integer>
	{
		using SysType = int;
		static constexpr size_t hlslSize = sizeof(int);
		static constexpr const char* code = "IN";
	};

#define X(el)                           \
	ASSERT("Invalid Data Type : " #el); \
	PRIMITIVE_DATA_TYPES
#undef X
}

// Poly
class IPolymorphData
{
	friend class RawLayout;
	friend struct ExtraData;
private:
	// pImpl Idiom
	struct ImplBase
	{
		virtual ~ImplBase() = default;
	};

public:
	std::string GetSignature() const DEBUG_EXCEPT;
	size_t GetOffsetBegin() const DEBUG_EXCEPT;
	size_t GetOffsetEnd() const DEBUG_EXCEPT;
	size_t GetSizeInBytes() const DEBUG_EXCEPT;

	IPolymorphData& operator[](const std::string& key) DEBUG_EXCEPT;
	IPolymorphData& T() DEBUG_EXCEPT;
	const IPolymorphData& T() const DEBUG_EXCEPT;
	const IPolymorphData& operator[](const std::string& key) const DEBUG_EXCEPT;

	bool Exists() const noexcept;
	std::pair<size_t, const IPolymorphData*> CalculateIndexingOffset(size_t offset, size_t index) const DEBUG_EXCEPT;
	IPolymorphData& Add(custom::Type Type, std::string Key) DEBUG_EXCEPT;

	template<custom::Type T>
	IPolymorphData& Add(std::string Key) DEBUG_EXCEPT
	{
		return Add(T, std::move(Key));
	}

	// only works for Arrays; set the type and the # of elements
	IPolymorphData& Set(custom::Type Type, size_t Size) DEBUG_EXCEPT;

	template<custom::Type T>
	IPolymorphData& Set(size_t size) DEBUG_EXCEPT
	{
		return Set(T, size);
	}

	// returns offset of leaf types for read/write purposes w/ typecheck in Debug
	template<typename T>
	size_t Resolve() const DEBUG_EXCEPT
	{
		switch (type)
		{
#define X(el)                                                          \
		    case el:                                                   \
		    {                                                          \
		    	ASSERT(typeid(custom::Map<el>::SysType) == typeid(T)); \
		    	return *offset;                                        \
		    }                                                          \
		    PRIMITIVE_DATA_TYPES
#undef X
		default:
		{
			ASSERT(false && "Tried to resolve non-leaf element");
			return 0u;
		}
		}
	}

private:
	IPolymorphData() noexcept = default;
	IPolymorphData(custom::Type Type) DEBUG_EXCEPT;

	static size_t AdvanceToBoundary(size_t Offset) noexcept;
	static size_t AdvanceIfCrossesBoundary(size_t Offset, size_t Size) noexcept;
	static bool CrossesBoundary(size_t Offset, size_t Size) noexcept;
	static bool ValidateSymbolName(const std::string& name) noexcept;

	std::string getSignatureForStruct() const DEBUG_EXCEPT;
	std::string getSignatureForArray() const DEBUG_EXCEPT;
	size_t finalize(size_t OffsetIn) DEBUG_EXCEPT;
	size_t finalizeForStruct(size_t OffsetIn);
	size_t finalizeForArray(size_t OffsetIn);

private:
	custom::Type              type = custom::Type::Empty;
	std::optional<size_t>     offset;
	std::unique_ptr<ImplBase> pImplData; // Opaque Pointer
};

class Layout
{
	friend class RawBuffer;
	friend class LayoutCodex;

public:
	size_t GetSizeInBytes() const noexcept;
	std::string GetSignature() const DEBUG_EXCEPT;

protected:
	Layout(std::shared_ptr<IPolymorphData> pPolymorphicData) noexcept;

protected:
	std::shared_ptr<IPolymorphData> pPolymorphicData;
};

class RawLayout : public Layout
{
	friend class LayoutCodex;

public:
	RawLayout() noexcept;
	IPolymorphData& operator[](const std::string& key) DEBUG_EXCEPT;

	template<custom::Type T>
	IPolymorphData& Add(const std::string& key) DEBUG_EXCEPT
	{
		return pPolymorphicData->Add<T>(key);
	}

private:
	void clearData() noexcept;
	std::shared_ptr<IPolymorphData> moveData() noexcept;
};

class ManagedLayout : public Layout
{
	friend class LayoutCodex;
	friend class RawBuffer;

public:
	const IPolymorphData& operator[](const std::string& key) const DEBUG_EXCEPT;
	std::shared_ptr<IPolymorphData> ShareRoot() const noexcept;

private:
	ManagedLayout(std::shared_ptr<IPolymorphData> pPolymorphicData) noexcept;
	std::shared_ptr<IPolymorphData> attachData() const noexcept;
};

class RawBuffer
{
public:
	class ElementRef // BufferIndex, DataLibrarian, Dictionary
	{
		friend RawBuffer;
	public:
		bool Exists() const noexcept
		{
			return pPolymorphicData->Exists();
		}
		ElementRef operator[](const std::string& key) const DEBUG_EXCEPT
		{
			return { &(*pPolymorphicData)[key],pBytes,offset };
		}
		ElementRef operator[](size_t index) const DEBUG_EXCEPT
		{
			const auto indexingData = pPolymorphicData->CalculateIndexingOffset(offset, index);
			return { indexingData.second, pBytes, indexingData.first };
		}

		template<typename T>
		bool SetIfExists(const T& val) DEBUG_EXCEPT
		{
			if (Exists())
			{
				*this = val;
				return true;
			}
			return false;
		}

		template<typename T>
		operator T& () const DEBUG_EXCEPT
		{
			return *reinterpret_cast<T*>(pBytes + offset + pPolymorphicData->Resolve<T>());
		}

		template<typename T>
		T& operator=(const T& rhs) const DEBUG_EXCEPT
		{
			return static_cast<T&>(*this) = rhs;
		}

	private:
		ElementRef(const IPolymorphData* pPolymorphicData, char* pBytes, size_t offset) noexcept
			: offset(offset), pPolymorphicData(pPolymorphicData), pBytes(pBytes)
		{
		}

	private:
		const  IPolymorphData* pPolymorphicData;
		size_t                 offset;
		char* pBytes;
	};

	RawBuffer(RawLayout&& lay) DEBUG_EXCEPT;
	RawBuffer(const ManagedLayout& lay) DEBUG_EXCEPT;
	RawBuffer(ManagedLayout&& lay) DEBUG_EXCEPT;
	RawBuffer(const RawBuffer&) noexcept;
	RawBuffer(RawBuffer&&) noexcept;

	ElementRef operator[](const std::string& key) DEBUG_EXCEPT
	{
		return { &(*pData)[key],bytes.data(),0u };
	}

	const char* GetData()              const noexcept;
	size_t                GetSizeInBytes()       const noexcept;
	const IPolymorphData& GetRootLayoutElement() const noexcept;

	void CopyFrom(const RawBuffer&) DEBUG_EXCEPT;
	std::shared_ptr<IPolymorphData> ShareLayoutRoot() const noexcept;

private:
	std::shared_ptr<IPolymorphData> pData;
	std::vector<char> bytes;
};