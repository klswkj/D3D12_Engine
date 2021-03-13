#include "stdafx.h"
#include "GpuTime.h"
#include "Device.h"
#include "CommandContext.h"
#include "CommandQueueManager.h"

// be Initialized at Device.h

namespace GPUTime
{
    ID3D12QueryHeap* sm_pQueryHeap      = nullptr;
    ID3D12Resource*  sm_pReadBackBuffer = nullptr;
    float* sm_pTimeStampBuffer          = nullptr; // will be mapped
    uint64_t sm_Fence        = 0ull;
    uint32_t sm_MaxNumTimers = 0u;
    uint32_t sm_NumTimers    = 1u;
    float sm_ValidTimeStart = 0.0f;
    float sm_ValidTimeEnd   = 0.0f;
    double sm_GpuTickDelta  = 0.0f;
}

void GPUTime::ClockInitialize(uint32_t MaxNumTimers/*= 4096*/)
{
    uint64_t GpuFrequency;
    device::g_commandQueueManager.GetCommandQueue()->GetTimestampFrequency(&GpuFrequency);
    sm_GpuTickDelta = 1.0 / static_cast<double>(GpuFrequency);

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC BufferDesc;
    BufferDesc.Width = sizeof(uint64_t) * MaxNumTimers * 2;
    BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    BufferDesc.Height = 1;
    BufferDesc.Alignment = 0;
    BufferDesc.DepthOrArraySize = 1;
    BufferDesc.MipLevels = 1;
    BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    BufferDesc.SampleDesc.Count = 1;
    BufferDesc.SampleDesc.Quality = 0;
    BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ASSERT_HR(device::g_pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&sm_pReadBackBuffer)));

    sm_pReadBackBuffer->SetName(L"GpuTimeStamp Buffer");

    D3D12_QUERY_HEAP_DESC QueryHeapDesc;
    QueryHeapDesc.Count = MaxNumTimers * 2;
    QueryHeapDesc.NodeMask = 1;
    QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    ASSERT_HR(device::g_pDevice->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&sm_pQueryHeap)));
    sm_pQueryHeap->SetName(L"GpuTimeStamp QueryHeap");

    sm_MaxNumTimers = (uint32_t)MaxNumTimers;
}

void GPUTime::Shutdown()
{
    SafeRelease(sm_pReadBackBuffer);
    SafeRelease(sm_pQueryHeap);
}

uint32_t GPUTime::NewTimer()
{
    return sm_NumTimers++;
}

void GPUTime::StartTimer(const custom::CommandContext& Context, const uint8_t commandIndex, const uint32_t TimerIdx)
{
    Context.InsertTimeStamp(sm_pQueryHeap, TimerIdx * 2, commandIndex);
}

void GPUTime::StopTimer(const custom::CommandContext& Context, const uint8_t commandIndex, const uint32_t TimerIdx)
{
    Context.InsertTimeStamp(sm_pQueryHeap, TimerIdx * 2 + 1, commandIndex);
}

void GPUTime::BeginReadBack()
{
    device::g_commandQueueManager.WaitFenceValueCPUSide(sm_Fence);

    D3D12_RANGE Range;
    Range.Begin = 0;
    Range.End   = ((size_t)sm_NumTimers * 2ul) * sizeof(uint64_t);
    ASSERT_HR(sm_pReadBackBuffer->Map(0, &Range, reinterpret_cast<void**>(&sm_pTimeStampBuffer)));

    sm_ValidTimeStart = sm_pTimeStampBuffer[0];
    sm_ValidTimeEnd   = sm_pTimeStampBuffer[1];

    // On the first frame, with random values in the timestamp query heap, we can avoid a misstart.
    if (sm_ValidTimeEnd < sm_ValidTimeStart)
    {
        sm_ValidTimeStart = 0ull;
        sm_ValidTimeEnd   = 0ull;
    }
}

void GPUTime::EndReadBack()
{
    // Unmap with an empty range to indicate nothing was written by the CPU
    D3D12_RANGE EmptyRange = {};
    sm_pReadBackBuffer->Unmap(0, &EmptyRange);
    sm_pTimeStampBuffer = nullptr;

    custom::CommandContext& Context = custom::CommandContext::Begin(D3D12_COMMAND_LIST_TYPE_DIRECT, 1);
    Context.InsertTimeStamp(sm_pQueryHeap, 1, 0u);
    Context.ResolveTimeStamps(sm_pReadBackBuffer, sm_pQueryHeap, sm_NumTimers * 2, 0u);
    Context.InsertTimeStamp(sm_pQueryHeap, 0, 0u);
    sm_Fence = Context.Finish();
}

float GPUTime::GetTime(uint32_t TimerIdx)
{
    ASSERT(sm_pTimeStampBuffer != nullptr, "Time stamp readback buffer is not mapped.");
    ASSERT(TimerIdx < sm_NumTimers, "Invalid GPU timer index.");

    uint64_t TimeStamp1 = (uint64_t)sm_pTimeStampBuffer[TimerIdx * 2];
    uint64_t TimeStamp2 = (uint64_t)sm_pTimeStampBuffer[TimerIdx * 2 + 1];

    if ((TimeStamp1 < sm_ValidTimeStart) || 
        (sm_ValidTimeEnd < TimeStamp2)   || 
        (TimeStamp2 <= TimeStamp1))
    {
        return 0.0f;
    }

    return static_cast<float>(sm_GpuTickDelta * (TimeStamp2 - TimeStamp1));
}
