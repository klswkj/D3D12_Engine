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

    void BeginBlock(const std::wstring& name, custom::CommandContext* Context = nullptr);
    void EndBlock(custom::CommandContext* Context = nullptr);

    void DisplayFrameRate(TextContext& Text);
    void DisplayPerfGraph(custom::GraphicsContext& Text);
    void Display(TextContext& Text, float x, float y, float w, float h);
    bool IsPaused();
}

#ifdef RELEASE
class ScopedTimer
{
public:
    ScopedTimer(const std::wstring&) 
    {
    }
    ScopedTimer(const std::wstring&, CommandContext&) 
    {
    }
};
#else
class ScopedTimer
{
public:
    ScopedTimer(const std::wstring& name) : m_Context(nullptr)
    {
        Profiling::BeginBlock(name);
    }
    ScopedTimer(const std::wstring& name, custom::CommandContext& Context) : m_Context(&Context)
    {
        Profiling::BeginBlock(name, m_Context);
    }
    ~ScopedTimer()
    {
        Profiling::EndBlock(m_Context);
    }

private:
    custom::CommandContext* m_Context;
};
#endif
