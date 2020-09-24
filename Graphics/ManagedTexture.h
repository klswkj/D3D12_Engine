#pragma once
#include "stdafx.h"
#include "Texture.h"
#include "RenderingResource.h"

namespace custom
{
    class CommandContext;
}

class Entity;

class ManagedTexture : public custom::Texture, public RenderingResource
{
public:
    ManagedTexture(const std::wstring& FileName, UINT RootIndex = -1, UINT Offset = -1)
        : m_MapKey(FileName), m_RootParameterIndex(RootIndex), m_ShaderOffset(Offset), m_IsValid(true)
    {
    }

    void WaitForLoad(void) const;
    // void Unload(void);
    // void operator= (const custom::Texture& Texture);

    void SetToInvalidTexture(void);
    bool IsValid(void) const { return m_IsValid; }

    //////////////////////////////////////
    /////////////////////////////////////

    void SetRootIndex(UINT RootIndex, UINT Offset);
    void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
private:
    std::wstring m_MapKey;        // For deleting from the map later
    UINT m_RootParameterIndex = -1;
    UINT m_ShaderOffset = -1;
    bool m_IsValid;
};