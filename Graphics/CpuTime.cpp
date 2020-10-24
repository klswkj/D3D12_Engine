#include "stdafx.h"
#include "CpuTime.h"

namespace CPUTime
{
    static double sm_CpuTickDelta = 0.0f;

    // Query the performance counter frequency
    void Initialize()
    {
        LARGE_INTEGER frequency;
        ASSERT(TRUE == QueryPerformanceFrequency(&frequency), "Unable to query performance counter frequency");
        sm_CpuTickDelta = 1.0 / static_cast<double>(frequency.QuadPart);
    }

    // Query the current value of the performance counter
    uint64_t GetCurrentTick()
    {
        LARGE_INTEGER currentTick;
        ASSERT(TRUE == QueryPerformanceCounter(&currentTick), "Unable to query performance counter value");
        return static_cast<uint64_t>(currentTick.QuadPart);
    }

    void BusyLoopSleep(float SleepTime)
    {
        uint64_t finalTick = (uint64_t)((double)SleepTime / sm_CpuTickDelta) + GetCurrentTick();
        while (GetCurrentTick() < finalTick);
    }

    double TicksToSeconds(uint64_t TickCount)
    {
        return TickCount * sm_CpuTickDelta;
    }

    double TicksToMillisecs(uint64_t TickCount)
    {
        return TickCount * sm_CpuTickDelta * 1000.0;
    }

    double TimeBetweenTicks(uint64_t tick1, uint64_t tick2)
    {
        return TicksToSeconds(tick2 - tick1);
    }
}