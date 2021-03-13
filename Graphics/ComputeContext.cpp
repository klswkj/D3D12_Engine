#include "stdafx.h"
#include "ComputeContext.h"
#include "Device.h"
#include "Graphics.h"
#include "RootSignature.h"
#include "UAVBuffer.h"
#include "ColorBuffer.h"
#include "CommandContextManager.h"
#include "CommandSignature.h"
#include "DynamicDescriptorHeap.h"

// namespace-device
// CommandContextManager g_commandContextManager;

namespace custom
{
    STATIC ComputeContext& ComputeContext::Begin
    (
        uint8_t numCommandALs /*= 1*/
    )
    {
        ASSERT(numCommandALs < UCHAR_MAX);
        ComputeContext* NewContext = reinterpret_cast<ComputeContext*>(device::g_commandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_COMPUTE, numCommandALs));

        return *NewContext;
    }
    STATIC ComputeContext& ComputeContext::Resume(uint64_t contextID)
    {
        custom::CommandContext* ret = nullptr;
        ret = device::g_commandContextManager.GetRecordingContext(D3D12_COMMAND_LIST_TYPE_COMPUTE, contextID);
        ASSERT(ret && ret->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE);
        return *reinterpret_cast<custom::ComputeContext*>(ret);
    }
    STATIC ComputeContext& ComputeContext::GetRecentContext()
    {
        custom::CommandContext* ret = nullptr;
        ret = device::g_commandContextManager.GetRecentContext(D3D12_COMMAND_LIST_TYPE_COMPUTE);
        ASSERT(ret && ret->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE);
        return *reinterpret_cast<custom::ComputeContext*>(ret);
    }
    void ComputeContext::SetRootSignature(const RootSignature& customRS, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
		if (customRS.GetSignature() == m_GPUTaskFiberContexts[commandIndex].pCurrentComputeRS)
		{
			return;
		}

        m_pCommandLists[commandIndex]->SetComputeRootSignature(m_GPUTaskFiberContexts[commandIndex].pCurrentComputeRS = customRS.GetSignature());
        m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.ParseComputeRootSignature(customRS);
        m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.ParseComputeRootSignature(customRS);
        INCRE_DEBUG_SET_RS_COUNT;
    }

    void ComputeContext::SetRootSignatureRange(const RootSignature& customRS, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
    {
        ASSERT((startCommandIndex <= endCommandIndex) && (endCommandIndex < m_NumCommandPair));

        for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
        {
            if (customRS.GetSignature() == m_GPUTaskFiberContexts[i].pCurrentComputeRS)
            {
                continue;
            }

            m_pCommandLists[i]->SetComputeRootSignature(m_GPUTaskFiberContexts[i].pCurrentComputeRS = customRS.GetSignature());

            m_GPUTaskFiberContexts[i].DynamicViewDescriptorHeaps.ParseComputeRootSignature(customRS);
            m_GPUTaskFiberContexts[i].DynamicSamplerDescriptorHeaps.ParseComputeRootSignature(customRS);
            INCRE_DEBUG_SET_RS_COUNT_I;
        }
    }

    void ComputeContext::SetConstantArray(UINT rootIndex, UINT numConstants, const void* pConstants, UINT offset /* = 0*/, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstants(rootIndex, numConstants, pConstants, offset);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::SetConstant(UINT rootIndex, UINT val, UINT offset /* = 0*/, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val, offset);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::SetConstants(UINT rootIndex, UINT val1, UINT val2, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val1, 0);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val2, 1);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }
    void ComputeContext::SetConstants(UINT rootIndex, UINT val1, UINT val2, UINT val3, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val1, 0);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val2, 1);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val3, 2);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }
    void ComputeContext::SetConstants(UINT rootIndex, UINT val1, UINT val2, UINT val3, UINT val4, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val1, 0);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val2, 1);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val3, 2);
        m_pCommandLists[commandIndex]->SetComputeRoot32BitConstant(rootIndex, val4, 3);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::SetConstantBuffer(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRootConstantBufferView(rootIndex, CBV);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::SetDynamicConstantBufferView(UINT rootIndex, size_t bufferSize, const void* pBufferData, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(pBufferData != nullptr && HashInternal::IsAligned(pBufferData, 16) && CHECK_VALID_COMMAND_INDEX);
        LinearBuffer CPUBuffer = m_CPULinearAllocator.Allocate(bufferSize);
        SIMDMemCopy(CPUBuffer.pData, pBufferData, HashInternal::AlignUp(bufferSize, 16) >> 4);
        // memcpy(CPUBuffer.pData, pBufferData, bufferSize);
        m_pCommandLists[commandIndex]->SetComputeRootConstantBufferView(rootIndex, CPUBuffer.GPUAddress);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::SetDynamicSRV(UINT rootIndex, size_t bufferSize, const void* pBufferData, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(pBufferData != nullptr && HashInternal::IsAligned(pBufferData, 16) && CHECK_VALID_COMMAND_INDEX);
        LinearBuffer CPUBuffer = m_CPULinearAllocator.Allocate(bufferSize);
        SIMDMemCopy(CPUBuffer.pData, pBufferData, HashInternal::AlignUp(bufferSize, 16) >> 4);
        m_pCommandLists[commandIndex]->SetComputeRootShaderResourceView(rootIndex, CPUBuffer.GPUAddress);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::SetBufferSRV(UINT rootIndex, const UAVBuffer& SRV, UINT64 offset, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT((SRV.m_currentState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0 && CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRootShaderResourceView(rootIndex, SRV.GetGpuVirtualAddress() + offset);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::SetBufferUAV(UINT rootIndex, const UAVBuffer& UAV, UINT64 offset, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT((UAV.m_currentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0 && CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRootUnorderedAccessView(rootIndex, UAV.GetGpuVirtualAddress() + offset);
        INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
    }

    void ComputeContext::Dispatch(size_t groupCountX, size_t groupCountY, size_t groupCountZ, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        // SubmitResourceBarriers();
        m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.SetComputeRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
        m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.SetComputeRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
        m_pCommandLists[commandIndex]->Dispatch((UINT)groupCountX, (UINT)groupCountY, (UINT)groupCountZ);
    }

    void ComputeContext::Dispatch1D(size_t threadCountX, size_t groupSizeX, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        Dispatch(HashInternal::DivideByMultiple(threadCountX, groupSizeX), 1, 1, commandIndex); 
        INCRE_DEBUG_DRAW_JOB_COUNT;
    }

    void ComputeContext::Dispatch2D(size_t threadCountX, size_t threadCountY, size_t groupSizeX, size_t groupSizeY, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        Dispatch
        (
            HashInternal::DivideByMultiple(threadCountX, groupSizeX),
            HashInternal::DivideByMultiple(threadCountY, groupSizeY), 1,
            commandIndex
        );
        INCRE_DEBUG_DRAW_JOB_COUNT;
    }

    void ComputeContext::Dispatch3D(size_t threadCountX, size_t threadCountY, size_t threadCountZ, size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        Dispatch
        (
            HashInternal::DivideByMultiple(threadCountX, groupSizeX),
            HashInternal::DivideByMultiple(threadCountY, groupSizeY),
            HashInternal::DivideByMultiple(threadCountZ, groupSizeZ),
            commandIndex
        );
        INCRE_DEBUG_DRAW_JOB_COUNT;
    }

    void ComputeContext::SetDynamicDescriptor(const UINT rootIndex, const UINT offset, const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        SetDynamicDescriptors(rootIndex, offset, 1, &CPUHandle, commandIndex);
    }

    void ComputeContext::SetDynamicDescriptors(const UINT rootIndex, const UINT offset, const UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandles[], const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.SetComputeDescriptorHandles(rootIndex, offset, Count, CPUHandles);
    }

    void ComputeContext::SetDynamicSampler(const UINT rootIndex, const UINT offset, const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        SetDynamicSamplers(rootIndex, offset, 1, &CPUHandle, commandIndex);
    }

    void ComputeContext::SetDynamicSamplers(const UINT rootIndex, const UINT offset, const UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE CPUhandles[], const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.SetComputeDescriptorHandles(rootIndex, offset, Count, CPUhandles);
    }

    void ComputeContext::SetDescriptorTable(const UINT rootIndex, const D3D12_GPU_DESCRIPTOR_HANDLE firstGPUHandle, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        m_pCommandLists[commandIndex]->SetComputeRootDescriptorTable(rootIndex, firstGPUHandle);
    }

    void ComputeContext::DispatchIndirect(UAVBuffer& argumentBuffer, uint64_t argumentBufferOffset, const uint8_t commandIndex/* = 0u*/)
    {
        // ASSERT(CHECK_VALID_COMMAND_INDEX);
        // ExecuteIndirect(graphics::g_DispatchIndirectCommandSignature, argumentBuffer, argumentBufferOffset, commandIndex);
    }

    void ComputeContext::ExecuteIndirect
    (
        const CommandSignature& commandSignature,
        const UAVBuffer& argumentBuffer, const uint64_t argumentStartOffset /*= 0*/,
        const uint32_t maxCommands/*= 1*/, UAVBuffer* commandCounterBuffer /*= nullptr*/, const uint64_t counterOffset/*= 0*/,
        const uint8_t commandIndex
    )
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        // SubmitResourceBarriers();
        m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.SetComputeRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
        m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.SetComputeRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
        m_pCommandLists[commandIndex]->ExecuteIndirect(commandSignature.GetSignature(), maxCommands,
            argumentBuffer.GetResource(), argumentStartOffset,
            commandCounterBuffer == nullptr ? nullptr : commandCounterBuffer->GetResource(), counterOffset);
    }

    void ComputeContext::ClearUAV(const UAVBuffer& target, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
        // a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.UploadDirect(target.GetUAV(), commandIndex);
        const UINT ClearColor[4] = {};
        m_pCommandLists[commandIndex]->ClearUnorderedAccessViewUint(GpuVisibleHandle, target.GetUAV(), target.GetResource(), ClearColor, 0, nullptr);
        INCRE_DEBUG_ASYNC_THING_COUNT;
    }

    void ComputeContext::ClearUAV(const ColorBuffer& target, const uint8_t commandIndex/* = 0u*/)
    {
        ASSERT(CHECK_VALID_COMMAND_INDEX);
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV 
        // (because it essentially runs a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.UploadDirect(target.GetUAV(), commandIndex);
        CD3DX12_RECT ClearRect(0L, 0L, (LONG)target.GetWidth(), (LONG)target.GetHeight());

        const float* ClearColor = target.GetClearColor().GetPtr();
        m_pCommandLists[commandIndex]->ClearUnorderedAccessViewFloat
        (
            GpuVisibleHandle, target.GetUAV(), 
            target.GetResource(), ClearColor, 
            1, &ClearRect
        );
        INCRE_DEBUG_ASYNC_THING_COUNT;
    }
}