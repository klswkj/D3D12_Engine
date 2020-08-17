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
    ManagedTexture(const std::wstring& FileName, uint32_t RootIndexOffset = -1)
        : m_MapKey(FileName), m_ShaderInputFlag(RootIndexOffset), m_IsValid(true)
    {
    }

    void WaitForLoad(void) const;
    // void Unload(void);
    // void operator= (const custom::Texture& Texture);

    void SetToInvalidTexture(void);
    bool IsValid(void) const { return m_IsValid; }

    //////////////////////////////////////
    /////////////////////////////////////

    void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;

private:
    std::wstring m_MapKey;        // For deleting from the map later
    uint32_t m_ShaderInputFlag{ 0 };
    bool m_IsValid;
};