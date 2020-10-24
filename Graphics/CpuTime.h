#pragma once

// [Main function-side Timeline]
// SystemTime ¦¡¦¡ CpuTime 
//              

namespace CPUTime
{
    void Initialize(void);

    uint64_t GetCurrentTick(void);
    void BusyLoopSleep(float SleepTime);

    double TicksToSeconds(uint64_t TickCount);
    double TicksToMillisecs(uint64_t TickCount);
    double TimeBetweenTicks(uint64_t tick1, uint64_t tick2);
};