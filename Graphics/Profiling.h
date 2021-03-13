#pragma once
#include <string>
// #include "TextManager.h"

namespace custom
{
    class CommandContext;
    class GraphicsContext;
}

class TextContext;

namespace Profiling
{
    void Update();

    void BeginBlock(const std::wstring& name, custom::CommandContext* const Context, const uint8_t commandIndex);
    void EndBlock(custom::CommandContext* const Context, const uint8_t commandIndex);

    void DisplayFrameRate(TextContext& Text);
    void DisplayPerfGraph(custom::GraphicsContext& Text);
    void Display(TextContext& Text, float x, float y, float w, float h);
    bool IsPaused();
}

#ifdef RELEASE
class ScopedTimer
{
public:
    explicit ScopedTimer(const std::wstring&) 
    {
    }
    explicit ScopedTimer(const std::wstring&, CommandContext&) 
    {
    }
};
#else
class ScopedTimer
{
public:
    explicit ScopedTimer(const std::wstring& name) 
        : 
        m_Context(nullptr),
        m_CommandIndex(0u)
    {
        Profiling::BeginBlock(name, nullptr, 0u);
    }
    explicit ScopedTimer(const std::wstring& name, custom::CommandContext& Context, const uint8_t commandIndex) 
        : 
        m_Context(&Context),
        m_CommandIndex(commandIndex)
    {
        Profiling::BeginBlock(name, m_Context, m_CommandIndex);
    }
    ~ScopedTimer()
    {
        Profiling::EndBlock(m_Context, m_CommandIndex);
    }

private:
    custom::CommandContext* const m_Context;
    const uint8_t m_CommandIndex;
};
#endif
