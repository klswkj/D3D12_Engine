#pragma once
#include "stdafx.h"
#include "CommandContext.h"

namespace custom
{
    class UAVBuffer;
    class RootSignature;

    class ComputeContext final : public CommandContext
    {
    public:
        static ComputeContext& Begin(uint8_t numCommandALs);
        static ComputeContext& Resume(uint64_t contextID);
        static ComputeContext& GetRecentContext();

        void SetRootSignature(const RootSignature& customRS, const uint8_t commandIndex);
        void SetRootSignatureRange(const RootSignature& customRS, const uint8_t startCommandIndex, const uint8_t endCommandIndex);

        void SetConstantArray(UINT rootIndex, UINT numConstants, const void* pConstants, UINT offset, const uint8_t commandIndex);
        void SetConstant(UINT rootIndex, UINT val, UINT offset, const uint8_t commandIndex);
        void SetConstants(UINT rootIndex, UINT val1, UINT val2, const uint8_t commandIndex);
        void SetConstants(UINT rootIndex, UINT val1, UINT val2, UINT val3, const uint8_t commandIndex);
        void SetConstants(UINT rootIndex, UINT val1, UINT val2, UINT val3, UINT val4, const uint8_t commandIndex);

        void SetConstantBuffer(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV, const uint8_t commandIndex);
        void SetDynamicConstantBufferView(UINT rootIndex, size_t BufferSize, const void* pBufferData, const uint8_t commandIndex);
        void SetDynamicSRV(UINT rootIndex, size_t BufferSize, const void* pBufferData, const uint8_t commandIndex);
        void SetBufferSRV(UINT rootIndex, const UAVBuffer& SRV, UINT64 offset, const uint8_t commandIndex);
        void SetBufferUAV(UINT rootIndex, const UAVBuffer& UAV, UINT64 offset, const uint8_t commandIndex);
        void SetDescriptorTable(UINT rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstGPUHandle, const uint8_t commandIndex);

        void SetDynamicDescriptor(UINT rootIndex, UINT offset, D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, const uint8_t commandIndex);
        void SetDynamicDescriptors(UINT rootIndex, UINT offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[], const uint8_t commandIndex);
        void SetDynamicSampler(UINT rootIndex, UINT offset, D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, const uint8_t commandIndex);
        void SetDynamicSamplers(UINT rootIndex, UINT offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandles[], const uint8_t commandIndex);

        void Dispatch(size_t groupCountX = 1, size_t groupCountY = 1, size_t groupCountZ = 1, const uint8_t commandIndex = UCHAR_MAX);
        void Dispatch1D(size_t ThreadCountX, size_t groupSizeX = 64, const uint8_t commandIndex = UCHAR_MAX);
        void Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t groupSizeX = 8, size_t groupSizeY = 8, const uint8_t commandIndex = UCHAR_MAX);
        void Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ, const uint8_t commandIndex);
        void DispatchIndirect(UAVBuffer& argumentBuffer, uint64_t argumentBufferOffset, const uint8_t commandIndex);
        void ExecuteIndirect(const CommandSignature& commandSig, const UAVBuffer& argumentBuffer, const uint64_t argumentStartOffset = 0,
            const uint32_t maxCommands = 1, UAVBuffer* commandcounterBuffer = nullptr, const uint64_t counterOffset = 0, const uint8_t commandIndex = UCHAR_MAX);

        void ClearUAV(const UAVBuffer& target, const uint8_t commandIndex);
        void ClearUAV(const ColorBuffer& target, const uint8_t commandIndex);
    private:
    };
}