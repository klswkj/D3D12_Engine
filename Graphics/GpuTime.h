#pragma once

namespace custom
{
    class CommandContext;
}

namespace GPUTime
{
    void ClockInitialize(uint32_t MaxNumTimers = 4096u);
    void Shutdown();

    // Reserve a unique timer index
    uint32_t NewTimer();

    // Write start and stop time stamps on the GPU timeline
    void StartTimer(const custom::CommandContext& Context, const uint8_t commandIndex, const uint32_t TimerIdx);
    void StopTimer(const custom::CommandContext& Context, const uint8_t commandIndex, const uint32_t TimerIdx);

    // Bookend all calls to GetTime() with Begin/End which correspond to Map/Unmap.  
    // This needs to happen either at the very start or very end of a frame.
    void BeginReadBack();
    void EndReadBack();

    // Returns the time in milliseconds between start and stop queries
    float GetTime(uint32_t TimerIdx = 4096u);
}
