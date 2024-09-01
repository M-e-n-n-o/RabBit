#pragma once

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    class GpuResource;

    struct UploadAllocation
    {
        GpuResource*                resource;
        uint64_t					maxWriteSize;
        uint64_t					offset;
        D3D12_GPU_VIRTUAL_ADDRESS	address;			// Contains the offset
        uint8_t*                    cpuWriteAddress;	// Contains the offset
    };

    class UploadPage
    {
    public:
        UploadPage(const char* name, uint64_t size);
        ~UploadPage();

        void Reset();

        bool HasSpace(uint64_t size, uint64_t alignment);

        UploadAllocation Allocate(uint64_t size, uint64_t alignment);

    private:
        const char* m_Name;

        GpuResource*                m_UploadResource;
        D3D12_GPU_VIRTUAL_ADDRESS	m_GpuAddress;
        uint8_t*                    m_WriteAddress;
        uint64_t					m_UploadOffset;
        uint64_t					m_ResourceSize;
    };

    class UploadAllocator
    {
    public:
        UploadAllocator(const char* name, uint64_t page_size);
        ~UploadAllocator();

        void Reset();

        UploadAllocation Allocate(uint64_t size, uint64_t alignment);

    private:
        int32_t AddNewPage();

        const char*         m_Name;

        List<UploadPage*>	m_Pages;
        int32_t				m_CurrentPage;

        uint64_t			m_PageSize;

        const uint32_t		m_DeleteCheck = 10;
        uint32_t			m_ResetCounter;
        int32_t				m_LastHighestPage;
    };
}