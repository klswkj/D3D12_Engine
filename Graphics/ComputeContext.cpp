#include "stdafx.h"
#include "Device.h"
#include "Graphics.h"
#include "UAVBuffer.h"
#include "ColorBuffer.h"
#include "ComputeContext.h"
#include "CommandContextManager.h"
#include "CommandSignature.h"
#include "DynamicDescriptorHeap.h"

// namespace-device
// CommandContextManager g_commandContextManager;

namespace custom
{
    ComputeContext& ComputeContext::Begin(const std::wstring& ID, bool Async)
    {
        ComputeContext& NewContext = device::g_commandContextManager.AllocateContext
        (Async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)->GetComputeContext();

        NewContext.setName(ID);

        //if (0 < ID.length())
        ////{
        //    EngineProfiling::BeginBlock(ID, &NewContext);
        //}

        return NewContext;
    }

    void ComputeContext::SetRootSignature(const RootSignature& RootSig)
    {
		if (RootSig.GetSignature() == m_pCurrentComputeRootSignature)
		{
			return;
		}

        m_commandList->SetComputeRootSignature(m_pCurrentComputeRootSignature = RootSig.GetSignature());

        m_DynamicViewDescriptorHeap.ParseComputeRootSignature(RootSig);
        m_DynamicSamplerDescriptorHeap.ParseComputeRootSignature(RootSig);
    }

    void ComputeContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants, UINT Offset /* = 0*/)
    {
        m_commandList->SetComputeRoot32BitConstants(RootIndex, NumConstants, pConstants, Offset);
    }

    void ComputeContext::SetConstant(UINT RootIndex, UINT Val, UINT Offset /* = 0*/)
    {
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val, Offset);
    }

    void ComputeContext::SetConstants(UINT RootIndex, UINT Val1, UINT Val2)
    {
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val1, 0);
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val2, 1);
    }
    void ComputeContext::SetConstants(UINT RootIndex, UINT Val1, UINT Val2, UINT Val3)
    {
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val1, 0);
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val2, 1);
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val3, 2);
    }
    void ComputeContext::SetConstants(UINT RootIndex, UINT Val1, UINT Val2, UINT Val3, UINT Val4)
    {
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val1, 0);
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val2, 1);
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val3, 2);
        m_commandList->SetComputeRoot32BitConstant(RootIndex, Val4, 3);
    }

    void ComputeContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        m_commandList->SetComputeRootConstantBufferView(RootIndex, CBV);
    }

    void ComputeContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16));
        LinearBuffer cb = m_CPULinearAllocator.Allocate(BufferSize);
        SIMDMemCopy(cb.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
        // memcpy(cb.pData, BufferData, BufferSize);
        m_commandList->SetComputeRootConstantBufferView(RootIndex, cb.GPUAddress);
    }

    void ComputeContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16));
        LinearBuffer cb = m_CPULinearAllocator.Allocate(BufferSize);
        SIMDMemCopy(cb.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
        m_commandList->SetComputeRootShaderResourceView(RootIndex, cb.GPUAddress);
    }

    void ComputeContext::SetBufferSRV(UINT RootIndex, const UAVBuffer& SRV, UINT64 Offset)
    {
        ASSERT((SRV.m_currentState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
        m_commandList->SetComputeRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
    }

    void ComputeContext::SetBufferUAV(UINT RootIndex, const UAVBuffer& UAV, UINT64 Offset)
    {
        ASSERT((UAV.m_currentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
        m_commandList->SetComputeRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
    }

    void ComputeContext::Dispatch(size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_commandList);
        m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_commandList);
        m_commandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
    }

    void ComputeContext::Dispatch1D(size_t ThreadCountX, size_t GroupSizeX)
    {
        Dispatch(HashInternal::DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1);
    }

    void ComputeContext::Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY)
    {
        Dispatch
        (
            HashInternal::DivideByMultiple(ThreadCountX, GroupSizeX),
            HashInternal::DivideByMultiple(ThreadCountY, GroupSizeY), 1
        );
    }

    void ComputeContext::Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ)
    {
        Dispatch
        (
            HashInternal::DivideByMultiple(ThreadCountX, GroupSizeX),
            HashInternal::DivideByMultiple(ThreadCountY, GroupSizeY),
            HashInternal::DivideByMultiple(ThreadCountZ, GroupSizeZ)
        );
    }

    void ComputeContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
    }

    void ComputeContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_DynamicViewDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    void ComputeContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
    }

    void ComputeContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_DynamicSamplerDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    void ComputeContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        m_commandList->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
    }

    void ComputeContext::DispatchIndirect(UAVBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
    {
        ExecuteIndirect(graphics::g_DispatchIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
    }
    /*
    void ExecuteIndirect(CommandSignature& CommandSig, UAVBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
            uint32_t MaxCommands = 1, UAVBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);
    */
    void ComputeContext::ExecuteIndirect
    (
        CommandSignature& CommandSig,
        UAVBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset /*= 0*/,
        uint32_t MaxCommands/*= 1*/, UAVBuffer* CommandCounterBuffer /*= nullptr*/, uint64_t CounterOffset/*= 0*/
    )
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_commandList);
        m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_commandList);
        m_commandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
            ArgumentBuffer.GetResource(), ArgumentStartOffset,
            CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
    }

    void ComputeContext::ClearUAV(UAVBuffer& Target)
    {
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
        // a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        const UINT ClearColor[4] = {};
        m_commandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
    }

    void ComputeContext::ClearUAV(ColorBuffer& Target)
    {
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV 
        // (because it essentially runs a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        CD3DX12_RECT ClearRect(0L, 0L, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

        const float* ClearColor = Target.GetClearColor().GetPtr();
        m_commandList->ClearUnorderedAccessViewFloat
        (
            GpuVisibleHandle, Target.GetUAV(), 
            Target.GetResource(), ClearColor, 
            1, &ClearRect
        );
    }
}