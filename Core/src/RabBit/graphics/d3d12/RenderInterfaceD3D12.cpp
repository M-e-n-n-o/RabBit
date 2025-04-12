#include "RabBitCommon.h"
#include "RenderInterfaceD3D12.h"
#include "DeviceQueue.h"
#include "resource/ResourceManager.h"
#include "resource/RenderResourceD3D12.h"
#include "resource/ResourceStateManager.h"
#include "resource/UploadAllocator.h"
#include "resource/DescriptorManager.h"
#include "Pipeline.h"
#include "ShaderSystem.h"
#include "UtilsD3D12.h"
#include "GraphicsDevice.h"
#include "graphics/ResourceDefaults.h"

#define USE_PIX
#include <pix3.h>

namespace RB::Graphics::D3D12
{
    GpuGuardD3D12::GpuGuardD3D12(uint64_t fence_value, DeviceQueue* queue)
        : m_FenceValue(fence_value)
        , m_Queue(queue)
    {
    }

    bool GpuGuardD3D12::IsFinishedRendering()
    {
        return m_Queue->IsFenceReached(m_FenceValue);
    }

    void GpuGuardD3D12::WaitUntilFinishedRendering()
    {
        m_Queue->CpuWaitForFenceValue(m_FenceValue);
    }

    // ---------------------------------------------------------------------------
    //								GpuGuard
    // ---------------------------------------------------------------------------

    RenderInterfaceD3D12::RenderInterfaceD3D12(bool allow_only_copy_operations)
        : m_CopyOperationsOnly(allow_only_copy_operations)
        , m_RenderState()
        , m_CurrentCBVAllocator(nullptr)
    {
        if (allow_only_copy_operations)
            m_Queue = g_GraphicsDevice->GetCopyQueue();
        else
            m_Queue = g_GraphicsDevice->GetGraphicsQueue();

        SetNewCommandList();
        InvalidateState(true);
    }

    RenderInterfaceD3D12::~RenderInterfaceD3D12()
    {
        SAFE_DELETE(m_CurrentCBVAllocator);

        while (!m_AvailableCBVAllocators.empty())
        {
            SAFE_DELETE(m_AvailableCBVAllocators.front());
            m_AvailableCBVAllocators.pop();
        }

        for (int i = 0; i < m_InFlightCBVAllocators.size(); ++i)
        {
            SAFE_DELETE(m_InFlightCBVAllocators[i].allocator);
        }
    }

    void RenderInterfaceD3D12::InvalidateState(bool rebind_descriptor_heap)
    {
        m_RenderState.pendingClears.clear();
        m_RenderState = {};

        if (m_CopyOperationsOnly)
        {
            return;
        }

        if (rebind_descriptor_heap)
        {
            BindDescriptorHeaps();
        }

        ClearDrawResources();
    }

    Shared<GpuGuard> RenderInterfaceD3D12::ExecuteInternal()
    {
        // TODO Maybe do the ExecuteCommandLists on a separate thread in the future?
        uint64_t fence_value = m_Queue->ExecuteCommandList(m_CommandList);

        g_ResourceManager->OnCommandListExecute(m_Queue, fence_value);

        if (m_CurrentCBVAllocator)
        {
            m_InFlightCBVAllocators.push_back({ m_CurrentCBVAllocator, fence_value });

            m_CurrentCBVAllocator = nullptr;
        }

        SetNewCommandList();
        InvalidateState(true);

        return CreateShared<GpuGuardD3D12>(fence_value, m_Queue);
    }

    void RenderInterfaceD3D12::GpuWaitOn(GpuGuard* guard)
    {
        GpuGuardD3D12* d3d_guard = (GpuGuardD3D12*)guard;
        m_Queue->GpuWaitForFenceValue(d3d_guard->m_Queue->GetFence(), d3d_guard->m_FenceValue);
    }

    void RenderInterfaceD3D12::TransitionResource(RenderResource* resource, ResourceState state)
    {
        MarkResourceUsed(resource);

        g_ResourceStateManager->TransitionResource((GpuResource*)resource->GetNativeResource(), ConvertToD3D12ResourceState(state));
    }

    void RenderInterfaceD3D12::FlushResourceBarriers()
    {
        g_ResourceStateManager->FlushPendingTransitions(m_CommandList.Get());
    }

    void RenderInterfaceD3D12::FlushAllPending()
    {
        HandlePendingClears();
        FlushResourceBarriers();
    }

    void RenderInterfaceD3D12::SetRenderTarget(RenderResource* color_target)
    {
        RenderTargetBundle bundle = {};
        bundle.colorTargetsCount    = 1;
        bundle.colorTargets[0]      = (Texture2D*)color_target;
        bundle.depthStencilTarget   = nullptr;

        SetRenderTarget(&bundle);
    }

    void RenderInterfaceD3D12::SetRenderTarget(RenderTargetBundle* bundle)
    {
        uint32_t width = ((Texture2DD3D12*)bundle->colorTargets[0])->GetWidth();
        uint32_t height = ((Texture2DD3D12*)bundle->colorTargets[0])->GetHeight();

        D3D12_CPU_DESCRIPTOR_HANDLE color_handles[8];

        for (int i = 0; i < _countof(bundle->colorTargets); ++i)
        {
            if (i < bundle->colorTargetsCount)
            {
                Texture2DD3D12* tex = (Texture2DD3D12*)bundle->colorTargets[i];

                if (!tex->AllowedRenderTarget())
                {
                    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Texture is not a render target");
                    continue;
                }

                // This also waits until the resource has been created
                MarkResourceUsed(tex);

                color_handles[i] = tex->GetRenderTargetHandle();

                if (width != tex->GetWidth() || height != tex->GetHeight())
                {
                    RB_LOG_WARN(LOGTAG_GRAPHICS, "It is not really allowed to have multiple rendertargets bound with different resolutions, might work for debugging though");
                    width = Math::Max(width, tex->GetWidth());
                    height = Math::Max(height, tex->GetHeight());
                }


                TransitionResource(tex, ResourceState::RENDER_TARGET);

                m_RenderState.rtvFormats[i] = ConvertToDXGIFormat(tex->GetFormat());
            }
            else
            {
                m_RenderState.rtvFormats[i] = DXGI_FORMAT_UNKNOWN;
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE* depth_handle = nullptr;

        Texture2D* depth_stencil = bundle->depthStencilTarget;
        if (depth_stencil)
        {
            if (depth_stencil->AllowedDepthStencil())
            {
                // This also waits until the resource has been created
                MarkResourceUsed(depth_stencil);

                depth_handle = &((Texture2DD3D12*)depth_stencil)->GetDepthStencilTargetHandle();

                TransitionResource(depth_stencil, ResourceState::DEPTH_WRITE);

                m_RenderState.dsvFormat = ConvertToDXGIFormat(depth_stencil->GetFormat());
            }
            else
            {
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Texture is not a depth stencil target");
            }
        }

        m_CommandList->OMSetRenderTargets(bundle->colorTargetsCount, color_handles, false, depth_handle);

        m_RenderState.numRenderTargets = bundle->colorTargetsCount;

        // Also set the viewport
        Viewport vp;
        vp.left   = 0;
        vp.top    = 0;
        vp.width  = width;
        vp.height = height;
        SetViewport(vp);

        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::SetShaderResourceInput(RenderResource* resource, uint32_t slot)
    {
        TransitionResource(resource, ResourceState::PIXEL_SHADER_RESOURCE);
        MarkResourceUsed(resource);

        switch (resource->GetType())
        {
        case RenderResourceType::Texture2D:
        {
            RB_ASSERT_FATAL(LOGTAG_GRAPHICS, slot < _countof(m_RenderState.tex2DsrvHandles), "Shader resource input slot out of range");

            Texture2DD3D12* tex = (Texture2DD3D12*)resource;

            m_RenderState.tex2DsrvHandles[slot] = tex->GetReadHandle();
            m_RenderState.tex2DSRGBs[slot] = tex->GetColorSpace() == TextureColorSpace::sRGB;
        }
        break;

        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "This resource type is not yet supported as a shader input");
            break;
        }
    }

    void RenderInterfaceD3D12::ClearShaderResourceInput(uint32_t slot)
    {
        m_RenderState.tex2DsrvHandles[slot] = -1;
        m_RenderState.tex2DSRGBs[slot] = false;
    }

    void RenderInterfaceD3D12::SetConstantShaderData(uint32_t slot, void* data, uint32_t data_size)
    {
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, slot < _countof(m_RenderState.cbvAddresses), "Up the amount of possible CBV addresses");

        if (m_CurrentCBVAllocator == nullptr)
        {
            // Update the available allocators
            {
                auto itr = m_InFlightCBVAllocators.begin();
                while (itr != m_InFlightCBVAllocators.end())
                {
                    if (m_Queue->IsFenceReached(itr->fenceValue))
                    {
                        itr->allocator->Reset();
                        m_AvailableCBVAllocators.push(itr->allocator);
                        itr = m_InFlightCBVAllocators.erase(itr);
                    }
                    else
                    {
                        ++itr;
                    }
                }
            }

            if (m_AvailableCBVAllocators.empty())
            {
                m_CurrentCBVAllocator = new UploadAllocator("CBV Upload Allocator", k64KB);
            }
            else
            {
                m_CurrentCBVAllocator = m_AvailableCBVAllocators.front();
                m_AvailableCBVAllocators.pop();
            }
        }

        UploadAllocation allocation = m_CurrentCBVAllocator->Allocate(data_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        memcpy(allocation.cpuWriteAddress, data, data_size);

        m_RenderState.cbvAddresses[slot] = allocation.address;
    }

    void RenderInterfaceD3D12::SetVertexShader(uint32_t shader_index)
    {
        m_RenderState.vsShader = shader_index;
        m_RenderState.psoDirty = true;
        m_RenderState.rootSignatureDirty = true;
    }

    void RenderInterfaceD3D12::SetPixelShader(uint32_t shader_index)
    {
        m_RenderState.psShader = shader_index;
        m_RenderState.psoDirty = true;
        m_RenderState.rootSignatureDirty = true;
    }

    void RenderInterfaceD3D12::SetNewCommandList()
    {
        m_CommandList = m_Queue->GetCommandList();
    }

    void RenderInterfaceD3D12::Clear(RenderResource* resource, const Math::Float4& color)
    {
        if (resource->GetPrimitiveType() != RenderResourceType::Texture)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Could not clear RenderResource as its not a texture and textures are currently only supported for clears");
            return;
        }

        Texture* tex = ((Texture*)resource);

        // Delay the clears so that they can get batched together just before a draw/dispatch

        if (tex->AllowedRenderTarget())
        {
            MarkResourceUsed(resource);
            TransitionResource(resource, ResourceState::RENDER_TARGET);

            PendingClear clear = {};
            clear.renderTarget = true;
            clear.handle       = ((D3D12::Texture2DD3D12*)resource)->GetRenderTargetHandle();
            clear.color        = color;

            m_RenderState.pendingClears.push_back(clear);
        }
        else if (tex->AllowedDepthStencil())
        {
            MarkResourceUsed(resource);
            TransitionResource(resource, ResourceState::DEPTH_WRITE);

            PendingClear clear = {};
            clear.renderTarget = false;
            clear.handle       = ((D3D12::Texture2DD3D12*)resource)->GetDepthStencilTargetHandle();
            clear.color        = color;

            m_RenderState.pendingClears.push_back(clear);
        }
        else
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Could not clear RenderResource as its not a rendertarget or a depth stencil");
        }
    }

    void RenderInterfaceD3D12::SetViewport(const Viewport& viewport)
    {
        SetViewports(&viewport, 1);
    }

    void RenderInterfaceD3D12::SetViewports(const Viewport* viewports, uint32_t total_viewports)
    {
        D3D12_VIEWPORT* sizes = ALLOC_STACKC(D3D12_VIEWPORT, total_viewports);
        D3D12_RECT* rects = ALLOC_STACKC(D3D12_RECT, total_viewports);

        for (uint32_t i = 0; i < total_viewports; ++i)
        {
            sizes[i].TopLeftX   = viewports[i].left;
            sizes[i].TopLeftY   = viewports[i].top;
            sizes[i].Width      = viewports[i].width;
            sizes[i].Height     = viewports[i].height;
            sizes[i].MinDepth   = D3D12_MIN_DEPTH;
            sizes[i].MaxDepth   = D3D12_MAX_DEPTH;

            // Hardcoded scissor rects for now
            rects[i].left       = 0;
            rects[i].right      = LONG_MAX;
            rects[i].top        = 0;
            rects[i].bottom     = LONG_MAX;
        }

        m_CommandList->RSSetScissorRects(total_viewports, rects);
        m_CommandList->RSSetViewports(total_viewports, sizes);

        m_RenderState.scissorSet = true;
        m_RenderState.viewportSet = true;

        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::SetBlendMode(const BlendMode& mode)
    {
        D3D12_BLEND_DESC desc = {};

        switch (mode)
        {
        case BlendMode::None:
        {
            desc.AlphaToCoverageEnable = false;
            desc.IndependentBlendEnable = false;

            for (int i = 0; i < _countof(desc.RenderTarget); ++i)
            {
                desc.RenderTarget[i].BlendEnable = false;
                desc.RenderTarget[i].LogicOpEnable = false;
                desc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            }
        }
        break;

        case BlendMode::SrcAlphaLerp:
        {
            desc.AlphaToCoverageEnable = false;
            desc.IndependentBlendEnable = false;

            D3D12_RENDER_TARGET_BLEND_DESC rt_desc;
            rt_desc.BlendEnable             = true;
            rt_desc.LogicOpEnable           = false;
            rt_desc.SrcBlend                = D3D12_BLEND_SRC_ALPHA;
            rt_desc.DestBlend               = D3D12_BLEND_INV_SRC_ALPHA;
            rt_desc.BlendOp                 = D3D12_BLEND_OP_ADD;
            rt_desc.SrcBlendAlpha           = D3D12_BLEND_INV_DEST_ALPHA;
            rt_desc.DestBlendAlpha          = D3D12_BLEND_ONE;
            rt_desc.BlendOpAlpha            = D3D12_BLEND_OP_ADD;
            rt_desc.RenderTargetWriteMask   = D3D12_COLOR_WRITE_ENABLE_ALL;

            for (int i = 0; i < _countof(desc.RenderTarget); ++i)
            {
                desc.RenderTarget[i] = rt_desc;
            }
        }
        break;

        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Did not yet implement this blend mode for the RenderInterfaceD3D12");
            return;
        }

        m_RenderState.blendDesc = desc;
        m_RenderState.blendingSet = true;

        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::SetCullMode(const CullMode& mode)
    {
        D3D12_RASTERIZER_DESC desc = {};
        desc.FillMode               = D3D12_FILL_MODE_SOLID;
        desc.DepthClipEnable        = TRUE;
        desc.FrontCounterClockwise  = FALSE;
        desc.DepthBias              = D3D12_DEFAULT_DEPTH_BIAS;
        desc.DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.MultisampleEnable      = FALSE;
        desc.AntialiasedLineEnable  = FALSE;
        desc.ForcedSampleCount      = 0;
        desc.ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        switch (mode)
        {
        case CullMode::None:  desc.CullMode = D3D12_CULL_MODE_NONE;  break;
        case CullMode::Front: desc.CullMode = D3D12_CULL_MODE_FRONT; break;
        case CullMode::Back:  desc.CullMode = D3D12_CULL_MODE_BACK;  break;

        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Did not yet implement this cull mode for the RenderInterfaceD3D12");
            return;
        }

        m_RenderState.rasterizerSet = true;
        m_RenderState.rasterizerDesc = desc;

        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::SetDepthMode(const DepthMode& mode, bool write_depth, bool reversed_depth)
    {
        D3D12_DEPTH_STENCIL_DESC desc = {};
        desc.StencilEnable                  = FALSE;
        desc.StencilReadMask                = 0;
        desc.StencilWriteMask               = 0;
        //desc.FrontFace.StencilFailOp		= ;
        //desc.FrontFace.StencilDepthFailOp	= ;
        //desc.FrontFace.StencilPassOp		= ;
        //desc.FrontFace.StencilFunc		= ;
        //desc.BackFace.StencilFailOp		= ;
        //desc.BackFace.StencilDepthFailOp	= ;
        //desc.BackFace.StencilPassOp		= ;
        //desc.BackFace.StencilFunc			= ;

        switch (mode)
        {
        case DepthMode::PassAll:
        {
            desc.DepthEnable    = write_depth ? TRUE : FALSE;
            desc.DepthWriteMask = write_depth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc      = D3D12_COMPARISON_FUNC_ALWAYS;
        }
        break;

        case DepthMode::PassCloser:
        {
            desc.DepthEnable    = TRUE;
            desc.DepthWriteMask = write_depth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc      = reversed_depth ? D3D12_COMPARISON_FUNC_GREATER : D3D12_COMPARISON_FUNC_LESS;
        }
        break;

        case DepthMode::PassFurther:
        {
            desc.DepthEnable    = TRUE;
            desc.DepthWriteMask = write_depth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc      = reversed_depth ? D3D12_COMPARISON_FUNC_LESS : D3D12_COMPARISON_FUNC_GREATER;
        }
        break;

        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Did not yet implement this depth mode for the RenderInterfaceD3D12");
            return;
        }

        m_RenderState.depthStencilSet = true;
        m_RenderState.depthStencilDesc = desc;

        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::SetIndexBuffer(RenderResource* index_resource)
    {
        MarkResourceUsed(index_resource);

        IndexBufferD3D12* ib = (IndexBufferD3D12*)index_resource;

        m_CommandList->IASetIndexBuffer(&ib->GetView());

        m_RenderState.indexCountPerInstance = ib->GetIndexCount();
    }

    void RenderInterfaceD3D12::SetVertexBuffer(RenderResource* vertex_resource, uint32_t slot)
    {
        RenderResource* resources[] = { vertex_resource };
        SetVertexBuffers(resources, 1, slot);
    }

    void RenderInterfaceD3D12::SetVertexBuffers(RenderResource** vertex_resources, uint32_t resource_count, uint32_t start_slot)
    {
        D3D12_VERTEX_BUFFER_VIEW* views = (D3D12_VERTEX_BUFFER_VIEW*)ALLOC_STACK(sizeof(D3D12_VERTEX_BUFFER_VIEW) * resource_count);

        D3D_PRIMITIVE_TOPOLOGY type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

        for (uint32_t res_idx = 0; res_idx < resource_count; ++res_idx)
        {
            if (vertex_resources[res_idx]->GetType() != RenderResourceType::VertexBuffer)
            {
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Passed in resource %d was not a vertex buffer", res_idx);
                return;
            }

            VertexBufferD3D12* vbo = (VertexBufferD3D12*)vertex_resources[res_idx];

            MarkResourceUsed(vbo);

            views[res_idx] = vbo->GetView();

            if (type != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
            {
                RB_ASSERT_FATAL(LOGTAG_GRAPHICS, type == ConvertToD3D12Topology(vbo->GetTopologyType()), "The topology types of the passed in vertex buffers do not match");
            }
            else
            {
                type = ConvertToD3D12Topology(vbo->GetTopologyType());
            }
        }

        m_CommandList->IASetVertexBuffers(start_slot, resource_count, views);
        m_CommandList->IASetPrimitiveTopology(type);

        VertexBufferD3D12* base_vbo = ((VertexBufferD3D12*)vertex_resources[0]);

        m_RenderState.vertexCountPerInstance = base_vbo->GetVertexElementCount();

        D3D12_PRIMITIVE_TOPOLOGY_TYPE current_type = m_RenderState.vertexBufferType;

        switch (base_vbo->GetTopologyType())
        {
        case TopologyType::TriangleStrip:
        case TopologyType::TriangleList:	m_RenderState.vertexBufferType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;

        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Topology type not yet implemented");
            m_RenderState.vertexBufferType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
            break;
        }

        if (current_type != m_RenderState.vertexBufferType)
        {
            m_RenderState.psoDirty = true;
        }
    }

    void RenderInterfaceD3D12::CopyResource(RenderResource* src, RenderResource* dest)
    {
        if (src->GetType() != dest->GetType())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Can not copy resource as the typed do not match");
            return;
        }

        GpuResource* src_res = (GpuResource*)src->GetNativeResource();
        GpuResource* dest_res = (GpuResource*)dest->GetNativeResource();

        InternalCopy(src_res, dest_res, src->GetPrimitiveType());
    }

    void RenderInterfaceD3D12::UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size)
    {
        // TODO Create a big upload resource and keep it alive for a couple of frames so we don't have to create an upload resource for every upload 
        // if we do 10 uploads after eachother (so suballocate an upload resource in the full resource). If we create such a big upload resource, 
        // make sure that when doing texture uploads, the starting point of a allocation should be aligned by D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT!

        RB_ASSERT(LOGTAG_GRAPHICS, m_CopyOperationsOnly, "This operation should only be done on a Copy Queue!");

        switch (resource->GetPrimitiveType())
        {
        case RenderResourceType::Buffer:
        {
            GpuResource* upload_res = new GpuResource();
            g_ResourceManager->ScheduleCreateUploadResource(upload_res, "Upload resource", { data_size });

            char* mapped_mem;
            RB_ASSERT_FATAL_D3D(upload_res->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mapped_mem)), "Could not map the upload resource");

            memcpy(mapped_mem, data, data_size);

            // Unmapping is unnecessary
            //upload_res->Unmap(0, nullptr);

            InternalCopy(upload_res, (GpuResource*)resource->GetNativeResource(), resource->GetPrimitiveType());

            delete upload_res;
        }
        break;

        case RenderResourceType::Texture:
        {
            // Reference: https://alextardif.com/D3D11To12P3.html

            D3D12_RESOURCE_DESC desc = ((GpuResource*)resource->GetNativeResource())->GetResource()->GetDesc();

            uint64_t row_size = GetElementSizeFromFormat(resource->GetFormat()) * desc.Width;

            uint64_t tex_mem_size = 0;
            uint32_t num_rows[MAX_TEXTURE_SUBRESOURCE_COUNT];
            uint64_t row_sizes_in_bytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];
            const uint64_t num_sub_resources = desc.MipLevels * desc.DepthOrArraySize;

            g_GraphicsDevice->Get()->GetCopyableFootprints(&desc, 0, (uint32_t)num_sub_resources, 0, layouts, num_rows, row_sizes_in_bytes, &tex_mem_size);

            GpuResource* upload_res = new GpuResource();
            g_ResourceManager->ScheduleCreateUploadResource(upload_res, "Upload resource", { tex_mem_size });

            uint8_t* mapped_mem;
            RB_ASSERT_FATAL_D3D(upload_res->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mapped_mem)), "Could not map the upload resource");

            for (uint64_t array_index = 0; array_index < desc.DepthOrArraySize; array_index++)
            {
                for (uint64_t mip_index = 0; mip_index < desc.MipLevels; mip_index++)
                {
                    const uint64_t sub_resource_index = mip_index + (array_index * desc.MipLevels);

                    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& sub_resourceLayout = layouts[sub_resource_index];

                    const uint64_t sub_resource_height = num_rows[sub_resource_index];
                    const uint64_t sub_resource_pitch = Math::AlignUp(sub_resourceLayout.Footprint.RowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
                    const uint64_t sub_resource_depth = sub_resourceLayout.Footprint.Depth;

                    uint8_t* destination_sub_resource_memory = mapped_mem + sub_resourceLayout.Offset;

                    for (uint64_t slice_index = 0; slice_index < sub_resource_depth; slice_index++)
                    {
                        //const DirectX::Image* sub_image = image_data->GetImage(mip_index, array_index, slice_index);

                        // TODO This will break with more than 1 subresource, fix when implementing mips!
                        const uint8_t* source_sub_resource_memory = ((uint8_t*)data);
                        uint64_t sub_resource_row_pitch = row_size;

                        for (uint64_t height = 0; height < sub_resource_height; height++)
                        {
                            memcpy(destination_sub_resource_memory, source_sub_resource_memory, Math::Min(sub_resource_pitch, sub_resource_row_pitch));
                            destination_sub_resource_memory += sub_resource_pitch;
                            source_sub_resource_memory += sub_resource_row_pitch;
                        }
                    }
                }
            }

            for (int sub_resource_index = 0; sub_resource_index < num_sub_resources; ++sub_resource_index)
            {
                D3D12_TEXTURE_COPY_LOCATION src_loc = {};
                src_loc.pResource               = upload_res->GetResource().Get();
                src_loc.Type                    = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                src_loc.PlacedFootprint         = layouts[sub_resource_index];
                src_loc.PlacedFootprint.Offset  = 0;

                D3D12_TEXTURE_COPY_LOCATION dest_loc = {};
                dest_loc.pResource          = ((GpuResource*)resource->GetNativeResource())->GetResource().Get();
                dest_loc.Type               = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dest_loc.SubresourceIndex   = sub_resource_index;

                m_CommandList->CopyTextureRegion(&dest_loc, 0, 0, 0, &src_loc, nullptr);
            }

            MarkResourceUsed(resource);
            MarkResourceUsed(upload_res);

            delete upload_res;
        }
        break;

        default:
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Upload not possible for this type");
            break;
        }
    }

    void RenderInterfaceD3D12::DrawInternal()
    {
        HandlePendingClears();

        if (m_RenderState.psoDirty || m_RenderState.rootSignatureDirty)
        {
            SetGraphicsPipelineState();
        }

        BindDrawResources();

        FlushResourceBarriers();

        if (m_RenderState.indexCountPerInstance > 0)
        {
            m_CommandList->DrawIndexedInstanced(m_RenderState.indexCountPerInstance, 1, 0, 0, 0);
        }
        else
        {
            m_CommandList->DrawInstanced(m_RenderState.vertexCountPerInstance, 1, 0, 0);
        }
    }

    void RenderInterfaceD3D12::DispatchInternal()
    {
        // TODO Auto place UAV barriers if needed (also when doing a UAV clear)

        //HandlePendingClears();
    }

    void RenderInterfaceD3D12::ProfileMarkerBegin(uint64_t color, const char* name)
    {
        PIXBeginEvent(m_CommandList.Get(), color, name);
    }

    void RenderInterfaceD3D12::ProfileMarkerEnd()
    {
        PIXEndEvent(m_CommandList.Get());
    }

    void RenderInterfaceD3D12::BindDescriptorHeaps()
    {
        uint32_t num_heaps;
        auto heaps = g_DescriptorManager->GetHeaps(num_heaps);
        m_CommandList->SetDescriptorHeaps(num_heaps, heaps.data());
    }

    void RenderInterfaceD3D12::HandlePendingClears()
    {
        if (m_RenderState.pendingClears.empty())
        {
            return;
        }

        FlushResourceBarriers();

        for (uint32_t i = 0; i < m_RenderState.pendingClears.size(); ++i)
        {
            PendingClear clear = m_RenderState.pendingClears[i];

            if (clear.renderTarget)
            {
                FLOAT clear_color[] = { clear.color.r, clear.color.g, clear.color.b, clear.color.a };
                m_CommandList->ClearRenderTargetView(clear.handle, clear_color, 0, nullptr);
            }
            else
            {
                m_CommandList->ClearDepthStencilView(clear.handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_DEPTH, clear.color.r, clear.color.g, 0, nullptr);
            }
        }
    }

    void RenderInterfaceD3D12::InternalCopy(GpuResource* src, GpuResource* dst, const RenderResourceType& primitive_type)
    {
        MarkResourceUsed(src);
        MarkResourceUsed(dst);

        if (!m_CopyOperationsOnly)
        {
            g_ResourceStateManager->TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
            g_ResourceStateManager->TransitionResource(dst, D3D12_RESOURCE_STATE_COPY_DEST);
            FlushResourceBarriers();
        }
        else
        {
            // We do not need to transition resources to the copy state when using a dedicated copy commandlist.
            // The resources however MUST be in the COMMON state, so check for that here.
            RB_ASSERT_FATAL(LOGTAG_GRAPHICS, src->IsInState(D3D12_RESOURCE_STATE_COMMON), "Source resource was not in the common state before copying");
            RB_ASSERT_FATAL(LOGTAG_GRAPHICS, dst->IsInState(D3D12_RESOURCE_STATE_COMMON), "Destination resource was not in the common state before copying");
        }

        switch (primitive_type)
        {
        case RenderResourceType::Buffer: // Buffer -> Buffer copy
        {
            m_CommandList->CopyResource(dst->GetResource().Get(), src->GetResource().Get());
        }
        break;

        default:
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }
    }

    void RenderInterfaceD3D12::MarkResourceUsed(RenderResource* resource)
    {
        MarkResourceUsed((GpuResource*)resource->GetNativeResource());
    }

    void RenderInterfaceD3D12::MarkResourceUsed(GpuResource* resource)
    {
        // Wait until the resource is created
        resource->GetResource();

        resource->MarkAsUsed(m_Queue);
    }

    void RenderInterfaceD3D12::BindDrawResources()
    {
        // Set the bindless SRV/UAV slots (only if the shader is actually using any textures)
        if ((g_ShaderSystem->GetShaderResourceMask(m_RenderState.vsShader).cbvMask & (1 << kTexIndicesCB)) > 0 ||
            (g_ShaderSystem->GetShaderResourceMask(m_RenderState.psShader).cbvMask & (1 << kTexIndicesCB)) > 0)
        {
            TextureIndices indices = {};

            // Set the Texture2D's
            for (int i = 0; i < _countof(indices.tex2D); ++i)
            {
                uint32_t& index  = indices.tex2D[i].tableID;
                uint32_t& isSRGB = indices.tex2D[i].isSRGB;

                if (m_RenderState.tex2DsrvHandles[i] >= 0)
                {
                    index  = (uint32_t)m_RenderState.tex2DsrvHandles[i];
                    isSRGB = m_RenderState.tex2DSRGBs[i];
                }
                else
                {
                    // Error texture
                    index  = (uint32_t)((Texture2DD3D12*)g_TexDefaultError)->GetReadHandle();
                    isSRGB = false;
                }
            }

            SetConstantShaderData(kTexIndicesCB, &indices, sizeof(TextureIndices)); // TODO Make the texture indices a root constant instead of a CBV
        }

        // Set the bindless table
        m_CommandList->SetGraphicsRootDescriptorTable(BINDLESS_ROOT_PARAMETER_INDEX, g_DescriptorManager->GetTex2DStart());

        // Bind the CBV's
        uint32_t root_index = 0;
        for (int i = 0; i < _countof(m_RenderState.cbvAddresses); ++i)
        {
            if (m_RenderState.cbvAddresses[i] > 0)
            {
                m_CommandList->SetGraphicsRootConstantBufferView(CBV_ROOT_PARAMETER_INDEX_OFFSET + root_index, m_RenderState.cbvAddresses[i]);
                root_index++;
            }

#if RB_CONFIG_DEBUG
            // Some extra debug checks
            bool occupies_slot = ((g_ShaderSystem->GetShaderResourceMask(m_RenderState.vsShader).cbvMask & (1 << i)) > 0 ||
                (g_ShaderSystem->GetShaderResourceMask(m_RenderState.psShader).cbvMask & (1 << i)) > 0);

            RB_ASSERT(LOGTAG_GRAPHICS, m_RenderState.cbvAddresses[i] > 0 == occupies_slot, "The bounded CBV slot does not match the shader's used CBV slots");
#endif
        }
    }

    void RenderInterfaceD3D12::ClearDrawResources()
    {
        // Clear all SRV's
        for (int i = 0; i < _countof(m_RenderState.tex2DsrvHandles); ++i)
        {
            ClearShaderResourceInput(i);
        }
    }

    void RenderInterfaceD3D12::SetGraphicsPipelineState()
    {
        #define CHECK_SET(check, message) if (!(check)) { RB_LOG_ERROR(LOGTAG_GRAPHICS, message); return; }

        CHECK_SET(m_RenderState.vertexBufferType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,    "Cannot draw, vertex buffer was not set")
        CHECK_SET(m_RenderState.numRenderTargets > 0,                                           "Cannot draw, vertex buffer was not set")
        CHECK_SET(m_RenderState.scissorSet,                                                     "Cannot draw, scissor was not set")
        CHECK_SET(m_RenderState.viewportSet,                                                    "Cannot draw, viewport was not set")
        CHECK_SET(m_RenderState.vsShader >= 0 && m_RenderState.psShader >= 0,                   "Cannot draw, vertex or pixel shader was not set yet")
        CHECK_SET(m_RenderState.blendingSet,                                                    "Cannot draw, blend mode was not set")
        CHECK_SET(m_RenderState.rasterizerSet,                                                  "Cannot draw, rasterizer was not set")
        CHECK_SET(m_RenderState.depthStencilSet,                                                "Cannot draw, depth stencil was not set")

        #undef CHECK_SET

        if (m_RenderState.rootSignatureDirty)
        {
            m_RenderState.rootSignature = g_PipelineManager->GetRootSignature(m_RenderState.vsShader, m_RenderState.psShader);

            m_RenderState.rootSignatureDirty = false;
        }

        List<D3D12_INPUT_ELEMENT_DESC> input_elements = g_PipelineManager->GetInputElementDesc(m_RenderState.vsShader);

        CompiledShaderBlob* vs_blob = g_ShaderSystem->GetCompilerShader(m_RenderState.vsShader);
        CompiledShaderBlob* ps_blob = g_ShaderSystem->GetCompilerShader(m_RenderState.psShader);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        pso_desc.pRootSignature         = m_RenderState.rootSignature.Get();
        pso_desc.VS                     = { vs_blob->shaderBlob, vs_blob->shaderBlobSize };
        pso_desc.PS                     = { ps_blob->shaderBlob, ps_blob->shaderBlobSize };
        //pso_desc.DS					= ;
        //pso_desc.HS					= ;
        //pso_desc.GS					= ;
        //pso_desc.StreamOutput			= ;
        pso_desc.BlendState             = m_RenderState.blendDesc;
        pso_desc.SampleMask             = UINT_MAX;
        pso_desc.RasterizerState        = m_RenderState.rasterizerDesc;
        pso_desc.DepthStencilState      = m_RenderState.depthStencilDesc;
        pso_desc.InputLayout            = { input_elements.data(), (UINT)input_elements.size() };
        //pso_desc.IBStripCutValue		= ;
        pso_desc.PrimitiveTopologyType  = m_RenderState.vertexBufferType;
        pso_desc.NumRenderTargets       = m_RenderState.numRenderTargets;
        /*pso_desc.RTVFormats */		  memcpy(pso_desc.RTVFormats, m_RenderState.rtvFormats, sizeof(DXGI_FORMAT) * 8);
        pso_desc.DSVFormat              = m_RenderState.dsvFormat;
        pso_desc.SampleDesc             = { 1, 0 };
        pso_desc.NodeMask               = 0;
        //pso_desc.CachedPSO			= NULL;
        pso_desc.Flags                  = D3D12_PIPELINE_STATE_FLAG_NONE;

        GPtr<ID3D12PipelineState> pso = g_PipelineManager->GetGraphicsPipeline(pso_desc, m_RenderState.vsShader, m_RenderState.psShader);

        // All bound resources are not valid anymore after this
        m_CommandList->SetPipelineState(pso.Get());
        m_CommandList->SetGraphicsRootSignature(m_RenderState.rootSignature.Get());

        m_RenderState.psoDirty = false;
    }
}