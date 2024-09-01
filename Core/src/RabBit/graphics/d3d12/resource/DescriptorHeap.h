#pragma once

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    using DescriptorHandle = int32_t;

    class DescriptorHeap
    {
    public:
        DescriptorHeap(const wchar_t* name, bool shader_visible, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_descriptors);

        // (Shader visible heap only) Use this staging descriptor to create descriptors that will be inserted directly after creation into the main heap
        D3D12_CPU_DESCRIPTOR_HANDLE GetStagingDestinationDescriptor() const;

        // (Non shader visible heap only)
        D3D12_CPU_DESCRIPTOR_HANDLE GetDestinationDescriptor(DescriptorHandle& out_handle, uint32_t offset = 0, uint32_t max_descriptors_type = 0, DescriptorHandle* handle_overwrite = nullptr);

        // (Shader visible heap only) Call this to copy over the newly created non shader-visible descriptor over to the shader visible heap
        DescriptorHandle InsertStagedDescriptor(uint32_t offset = 0, uint32_t max_descriptors_type = 0, DescriptorHandle* handle_overwrite = nullptr);

        void InvalidateDescriptor(DescriptorHandle& handle);

        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuStart(uint32_t offset);
        GPtr<ID3D12DescriptorHeap> GetHeap() const { return m_MainHeap; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_Type; }

    private:
        uint32_t FindEmptyDescriptorSlot(uint32_t offset, uint32_t max_descriptors_type, DescriptorHandle* handle_overwrite);

        List<bool>					m_AvailableSlots;

        GPtr<ID3D12DescriptorHeap>	m_MainHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE m_MainStart;
        GPtr<ID3D12DescriptorHeap>	m_StagingHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE	m_StagingStart;
        D3D12_DESCRIPTOR_HEAP_TYPE	m_Type;
        bool						m_ShaderVisible;
        uint32_t					m_IncrementSize;
    };
}