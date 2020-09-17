#include <DirectXMath.h>

#include "CommandContext.h"

#include "RenderingResource.h"
#include "Entity.h"
#include "eLayoutFlags.h"
#include "MathBasic.h"
#include "Camera.h"
#include "ShadowCamera.h"

// RootIndex, RegisterNumber(Offset), D3D12_SHADER_VISIBILITY 

class BoolBuffer : public RenderingResource
{
public:
	BoolBuffer(const char* name, BOOL data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{
		custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

		// graphicsContext.SetDynamicConstantBufferView();
	}

	char* GetName() { return &m_Name[0]; }
	BOOL GetData() { return m_Data; }
	BOOL* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}
private:
	char m_Name[10];
	BOOL m_Data{ 0 };
};

class FloatBuffer : public RenderingResource
{
public:
	FloatBuffer(const char* name, float data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{

	}

	char* GetName() { return &m_Name[0]; }
	float GetData() { return m_Data; }
	float* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}

private:
	char m_Name[10];
	float m_Data{ 0.0f };
};

class Float2Buffer : public RenderingResource
{
public:
	Float2Buffer(const char* name, DirectX::XMFLOAT2 data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{

	}

	char* GetName() { return &m_Name[0]; }
	DirectX::XMFLOAT2 GetData() { return m_Data; }
	DirectX::XMFLOAT2* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}

private:
	char m_Name[10];
	DirectX::XMFLOAT2 m_Data{};
};

class Float3Buffer : public RenderingResource
{
public:
	Float3Buffer()
	{
		m_Name[0] = '\0';
		ZeroMemory(&m_Data, sizeof(m_Data));
	}

	Float3Buffer(const char* name, DirectX::XMFLOAT3 data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{
		custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
		graphicsContext.SetDynamicConstantBufferView(1, sizeof(m_Data), &m_Data);
	}

	void operator=(DirectX::XMFLOAT3 float3) { m_Data = float3; }

	char* GetName() { return &m_Name[0]; }
	DirectX::XMFLOAT3 GetData() { return m_Data; }
	DirectX::XMFLOAT3* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}

private:
	char m_Name[10];
	__declspec(align(16)) DirectX::XMFLOAT3 m_Data{};
};

class VSConstantBuffer : public RenderingResource
{
public:
	__declspec(align(16)) struct VSConstants
	{
		Math::Matrix4 model;
		Math::Matrix4 modelView;
		Math::Matrix4 modelViewProj;
	};

	VSConstantBuffer()
	{
		m_Name[0] = '\0';
		ZeroMemory(&m_Data, sizeof(m_Data));
	}

	VSConstantBuffer(const char* name, VSConstants data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{

	}

	char* GetName() { return &m_Name[0]; }
	VSConstants GetData() { return m_Data; }
	VSConstants* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}

private:
	char m_Name[10];
	VSConstants m_Data;
};

class Float4Buffer : public RenderingResource
{
public:
	Float4Buffer(const char* name, DirectX::XMFLOAT4 data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{

	}

	char* GetName() { return &m_Name[0]; }
	DirectX::XMFLOAT4 GetData() { return m_Data; }
	DirectX::XMFLOAT4* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}

private:
	char m_Name[10];
	__declspec(align(16)) DirectX::XMFLOAT4 m_Data{};
};

class Matrix4Buffer : public RenderingResource
{
public:
	Matrix4Buffer(const char* name, DirectX::XMMATRIX data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{

	}

	char* GetName() { return &m_Name[0]; }
	DirectX::XMMATRIX GetData() { return m_Data; }
	DirectX::XMMATRIX* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}

private:
	char m_Name[10];
	DirectX::XMMATRIX m_Data{};
};

class CustomBuffer : public RenderingResource
{
	__declspec(align(16)) struct MaterialData
	{
		BOOL UseGlossAlpha{ 0 };
		BOOL UseSpecularMap{ 0 };
		BOOL UseNormalMap{ 0 };
		float NormalMapWeight{ 0 };
		DirectX::XMFLOAT3 MaterialColor{ 0 };
	};
public:
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{
		custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

		// TODO : Set RootIndex
		graphicsContext.SetDynamicConstantBufferView(-1, sizeof(m_Data), &m_Data);
	}

	MaterialData m_Data;
};
/*
	struct VSConstants
		{
			DirectX::XMMATRIX model;
			DirectX::XMMATRIX modelView;
			DirectX::XMMATRIX modelViewProj;
		};
*/
class TransformBuffer : public RenderingResource
{
public:
	struct VSConstants
	{
		Math::Matrix4 modelToProjection;
		Math::Matrix4 modelToShadow;
		DirectX::XMFLOAT3 viewerPos;
	};
	
	TransformBuffer()
	{
		m_Name[0] = '\0';
		ZeroMemory(&m_Data, sizeof(m_Data));
	}

	// Constructor with Camera & ShadowCamera is deferred.

	TransformBuffer(const char* name, VSConstants data = {})
		: m_Data(data)
	{
		ASSERT(10 <= strnlen_s(name, 255));
		Rename(name);
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{
		BindAfterUpdate(BaseContext);
	}

	void BindAfterUpdate(custom::CommandContext& BaseContext)
	{
		ASSERT(m_pEntity != nullptr);
		ASSERT(m_pCamera != nullptr);
		ASSERT(m_pShadowCamera != nullptr);
		custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

		// m_pShadowCamera->Update(), m_pCamera->Update() 외부에서 했다고 가정.


		m_Data.modelToShadow = m_pShadowCamera->GetShadowMatrix();
		DirectX::XMStoreFloat3(&m_Data.viewerPos, m_pCamera->GetPosition());
		graphicsContext.SetDynamicConstantBufferView(0, sizeof(m_Data), &m_Data);
	}

	char* GetName() { return &m_Name[0]; }
	VSConstants GetData() { return m_Data; }
	VSConstants* GetDataPtr() { return &m_Data; }

	void Rename(const char* name)
	{
		ZeroMemory(&m_Name[0], sizeof(char) * 10);
		strcpy_s(&m_Name[0], 10, name);
	}

private:
	char m_Name[10];
	VSConstants m_Data;
	Entity* m_pEntity{ nullptr };
	Camera* m_pCamera{ nullptr };
	ShadowCamera* m_pShadowCamera{ nullptr };
};