#pragma once

// [Main function-side Timeline]
// SystemTime ¦¡¦¡ CpuTime 
//              

class SystemTime
{
public:
    static void Initialize(void);

    static uint64_t GetCurrentTick(void);

    static void BusyLoopSleep(float SleepTime);

    static inline double TicksToSeconds(uint64_t TickCount)
    {
        return TickCount * sm_CpuTickDelta;
    }

    static inline double TicksToMillisecs(uint64_t TickCount)
    {
        return TickCount * sm_CpuTickDelta * 1000.0;
    }

    static inline double TimeBetweenTicks(uint64_t tick1, uint64_t tick2)
    {
        return TicksToSeconds(tick2 - tick1);
    }

private:
    static double sm_CpuTickDelta; // 0.0f
};

class CpuTimer
{
public:

    CpuTimer()
    {
        m_StartTick = 0ull;
        m_ElapsedTicks = 0ull;
    }

    void Start()
    {
        if (m_StartTick == 0ull)
            m_StartTick = SystemTime::GetCurrentTick();
    }

    void Stop()
    {
        if (m_StartTick != 0ull)
        {
            m_ElapsedTicks += SystemTime::GetCurrentTick() - m_StartTick;
            m_StartTick = 0ull;
        }
    }

    void Reset()
    {
        m_ElapsedTicks = 0ull;
        m_StartTick = 0ull;
    }

    double GetTime() const
    {
        return SystemTime::TicksToSeconds(m_ElapsedTicks);
    }

private:
    uint64_t m_StartTick;
    uint64_t m_ElapsedTicks;
};