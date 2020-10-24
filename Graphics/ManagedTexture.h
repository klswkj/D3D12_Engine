#pragma once
#include "stdafx.h"
#include "Texture.h"

namespace custom
{
    class CommandContext;
}

class Entity;

class ManagedTexture : public custom::Texture
{
public:
    ManagedTexture(const std::wstring& FileName)
        : m_MapKey(FileName), m_IsValid(true)
    {
    }

    void WaitForLoad(void) const;
    // void Unload(void);
    // void operator= (const custom::Texture& Texture);

    void SetToInvalidTexture(void);
    bool IsValid(void) const { return m_IsValid; }

private:
    std::wstring m_MapKey;        // For deleting from the map later
    bool m_IsValid;
};