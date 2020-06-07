#pragma once
#include "stdafx.h"
#include "CommandContext.h"
#include "UAVBuffer.h"

namespace custom
{
    class ComputeContext : public CommandContext
    {
    public:
        static ComputeContext& Begin(const std::wstring& ID = L"", bool Async = false);

        void SetRootSignature(const RootSignature& RootSig);

        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetConstant(UINT RootIndex, UINT Val, UINT Offset = 0);
        void SetConstants(UINT RootIndex, uint32_t size, ...);

        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetBufferSRV(UINT RootIndex, const UAVBuffer& SRV, UINT64 Offset = 0);
        void SetBufferUAV(UINT RootIndex, const UAVBuffer& UAV, UINT64 Offset = 0);
        void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

        void Dispatch(size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1);
        void Dispatch1D(size_t ThreadCountX, size_t GroupSizeX = 64);
        void Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX = 8, size_t GroupSizeY = 8);
        void Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ);
        void DispatchIndirect(UAVBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
        void ExecuteIndirect(CommandSignature& CommandSig, UAVBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
            uint32_t MaxCommands = 1, UAVBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

        void ClearUAV(UAVBuffer& Target);
        void ClearUAV(ColorBuffer& Target);
    private:
    };
}