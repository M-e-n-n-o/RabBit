#if RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "RenderInterfaceD3D12.h"
#include "DeviceQueue.h"
#include "resource/ResourceManager.h"
#include "resource/RenderResourceD3D12.h"
#include "resource/ResourceStateManager.h"
#include "resource/UploadAllocator.h"
#include "resource/Descriptor.h"
#include "Pipeline.h"
#include "UtilsD3D12.h"
#include "GraphicsDevice.h"
#include "graphics/ResourceDefaults.h"
#include "graphics/Renderer.h"
#include "graphics/ShaderSystem.h"
#include "app/Application.h"
#include <ShaderReflection.h>

#define USE_PIX
#include <pix3.h>

#include <d3dx12/d3dx12.h>

namespace RB::Graphics::D3D12
{
    // ---------------------------------------------------------------------------
    //                                GpuGuard
    // ---------------------------------------------------------------------------

    GpuGuardD3D12::GpuGuardD3D12(uint64_t fence_value, DeviceQueue* queue)
        : m_FenceValue(fence_value)
        , m_Queue(queue)
    {
    }

    bool GpuGuardD3D12::IsFinishedRendering() const
    {
        return m_Queue->IsFenceReached(m_FenceValue);
    }

    void GpuGuardD3D12::WaitUntilFinishedRendering()
    {
        m_Queue->CpuWaitForFenceValue(m_FenceValue);
    }

    // ---------------------------------------------------------------------------
    //                             RenderInterface
    // ---------------------------------------------------------------------------

    RenderInterfaceD3D12::RenderInterfaceD3D12(bool allow_only_copy_operations)
        : m_CopyOperationsOnly(allow_only_copy_operations)
        , m_RenderState()
    {
        m_ShaderSystem = Application::GetInstance()->GetRenderer()->GetShaderSystem();

        if (allow_only_copy_operations)
            m_Queue = g_GraphicsDevice->GetCopyQueue();
        else
            m_Queue = g_GraphicsDevice->GetGraphicsQueue();

        SetNewCommandList();
        InvalidateState(true);
    }

    RenderInterfaceD3D12::~RenderInterfaceD3D12()
    {
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

        ClearConstantShaderData();
        ClearResourceInputs();
        ClearRenderTargets();
    }

    Shared<GpuGuard> RenderInterfaceD3D12::ExecuteInternal()
    {
        if (!m_CopyOperationsOnly)
        {
            // Copy queue should not (have to) handle transitions
            FlushAllPending();
        }

        // TODO Maybe do the ExecuteCommandLists on a separate thread in the future?
        uint64_t fence_value = m_Queue->ExecuteCommandList(m_CommandList);

        SetNewCommandList();
        InvalidateState(true);

        Shared<GpuGuardD3D12> guard = CreateShared<GpuGuardD3D12>(fence_value, m_Queue);

        for (ReadbackBuffer* buf : m_SchedulesReadbacks)
        {
            ((ReadbackBufferD3D12*)buf)->OnScheduledReadback(guard);
        }
        m_SchedulesReadbacks.clear();

        return guard;
    }

    void RenderInterfaceD3D12::GpuWaitOn(GpuGuard* guard)
    {
        GpuGuardD3D12* d3d_guard = (GpuGuardD3D12*)guard;
        m_Queue->GpuWaitForFenceValue(d3d_guard->m_Queue->GetFence(), d3d_guard->m_FenceValue);
    }

    void RenderInterfaceD3D12::TransitionResource(RenderResource* resource, ResourceState state)
    {
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

        if (m_RenderState.renderTargetDirty)
        {
            SetRenderTargets();
        }
    }

    void RenderInterfaceD3D12::PushRenderTarget(RenderResource* color_target, uint32_t index)
    {
        Texture* tex = (Texture*)color_target;
        if (!tex->AllowedRenderTarget())
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Texture is not a render target");
            return;
        }

        if (m_RenderState.width != tex->GetViewportWidth() || m_RenderState.height != tex->GetViewportHeight())
        {
            m_RenderState.width = tex->GetViewportWidth();
            m_RenderState.height = tex->GetViewportHeight();

            m_RenderState.viewportSet = false;
            m_RenderState.scissorSet = false;
        }

        // This also waits until the resource has been created
        TransitionResource(tex, ResourceState::RENDER_TARGET);

        if (tex->GetType() == RenderResourceType::Texture2D)
            m_RenderState.rtvHandles[index].push(((Texture2DD3D12*)tex)->GetRenderTargetHandle());
        else if (tex->GetType() == RenderResourceType::Texture2DArray)
            m_RenderState.rtvHandles[index].push(((Texture2DArrayD3D12*)tex)->GetRenderTargetHandle());
        else
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "ResourceType not yet supported as rendertarget");

        m_RenderState.rtvFormats[index].push(ConvertToDXGIFormat(tex->GetFormat(), true, false));

        m_RenderState.numRenderTargets = Math::Max(m_RenderState.numRenderTargets, index + 1);

        m_RenderState.renderTargetDirty = true;
        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::PopRenderTarget(uint32_t index)
    {
        if (m_RenderState.rtvHandles[index].size() == 0)
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "No rendertarget to pop at index %d", index);
            return;
        }

        m_RenderState.rtvHandles[index].pop();
        m_RenderState.rtvFormats[index].pop();

        if (index + 1 >= m_RenderState.numRenderTargets && m_RenderState.rtvHandles[index].size() == 0)
        {
            // If there are not RTV's left at this index, lower the number of bound rendertargets
            m_RenderState.numRenderTargets = index;
        }

        m_RenderState.renderTargetDirty = true;
        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::SetDepthStencil(RenderResource* ds_target)
    {
        if (ds_target->GetPrimitiveType() != RenderResourceType::Texture)
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Depth Stencil should be a Texture");
            return;
        }

        Texture* depth_stencil = (Texture*)ds_target;
        if (depth_stencil->AllowedDepthStencil())
        {
            if (m_RenderState.width != depth_stencil->GetViewportWidth() || m_RenderState.height != depth_stencil->GetViewportHeight())
            {
                m_RenderState.width = depth_stencil->GetViewportWidth();
                m_RenderState.height = depth_stencil->GetViewportHeight();

                m_RenderState.viewportSet = false;
                m_RenderState.scissorSet = false;
            }

            // This also waits until the resource has been created
            TransitionResource(depth_stencil, ResourceState::DEPTH_WRITE);

            if (depth_stencil->GetType() == RenderResourceType::Texture2D)
                m_RenderState.dsvHandle = ((Texture2DD3D12*)depth_stencil)->GetDepthStencilTargetHandle();
            else if (depth_stencil->GetType() == RenderResourceType::Texture2DArray)
                m_RenderState.dsvHandle = ((Texture2DArrayD3D12*)depth_stencil)->GetDepthStencilTargetHandle();
            else
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "ResourceType not yet supported as depth stencil");

            m_RenderState.dsvFormat = ConvertToDXGIFormat(depth_stencil->GetFormat(), true, true);

            m_RenderState.renderTargetDirty = true;
            m_RenderState.psoDirty = true;
        }
        else
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Texture is not a depth stencil target");
        }
    }

    void RenderInterfaceD3D12::ClearRenderTargets()
    {
        for (int i = 0; i < _countof(m_RenderState.rtvFormats); i++)
        {
            m_RenderState.rtvFormats[i] = Stack<DXGI_FORMAT>();
            m_RenderState.rtvHandles[i] = Stack<D3D12_CPU_DESCRIPTOR_HANDLE>();
        }

        m_RenderState.psoDirty = true;
        m_RenderState.numRenderTargets = 0;
        m_RenderState.renderTargetDirty = true;
        m_RenderState.viewportSet = false;
        m_RenderState.scissorSet = false;
        m_RenderState.width = 0;
        m_RenderState.height = 0;
        m_RenderState.dsvFormat = DXGI_FORMAT_UNKNOWN;
    }

    void RenderInterfaceD3D12::SetShaderResourceInput(uint32_t handle, RenderResource* resource)
    {
        TransitionResource(resource, ResourceState::READ);

        uint32_t binding_offset = (handle >> 3) & 0x1FFFFFFF;
        ShaderCompiler::Stage stage = (ShaderCompiler::Stage)(handle & 0x7);

        // Descriptor handle takes up a uint2 in Slang
        uint32_t slot = binding_offset / sizeof(uint64_t);

        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, slot < _countof(m_RenderState.vertexResourceHandles), "Shader resource input slot out of range");

        switch (resource->GetType())
        {
        case RenderResourceType::Texture2D:
        {

            switch (stage)
            {
            case RB::ShaderCompiler::Stage::kVertex:    m_RenderState.vertexResourceHandles[slot] = ((Texture2DD3D12*)resource)->GetSrvHandle(); break;
            case RB::ShaderCompiler::Stage::kPixel:     m_RenderState.pixelResourceHandles[slot] = ((Texture2DD3D12*)resource)->GetSrvHandle(); break;
            case RB::ShaderCompiler::Stage::kCompute:   m_RenderState.computeResourceHandles[slot] = ((Texture2DD3D12*)resource)->GetSrvHandle(); break;
            default:
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Shader stage not recognized as shader resource input");
                break;
            }
        }
        break;

        case RenderResourceType::Texture2DArray:
        {
            switch (stage)
            {
            case RB::ShaderCompiler::Stage::kVertex:    m_RenderState.vertexResourceHandles[slot] = ((Texture2DArrayD3D12*)resource)->GetSrvHandle(); break;
            case RB::ShaderCompiler::Stage::kPixel:     m_RenderState.pixelResourceHandles[slot] = ((Texture2DArrayD3D12*)resource)->GetSrvHandle(); break;
            case RB::ShaderCompiler::Stage::kCompute:   m_RenderState.computeResourceHandles[slot] = ((Texture2DArrayD3D12*)resource)->GetSrvHandle(); break;
            default:
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Shader stage not recognized as shader resource input");
                break;
            }
        }
        break;

        case RenderResourceType::GenericBuffer:
        {
            switch (stage)
            {
            case RB::ShaderCompiler::Stage::kVertex:    m_RenderState.vertexResourceHandles[slot] = ((GenericBufferD3D12*)resource)->GetSrvHandle(); break;
            case RB::ShaderCompiler::Stage::kPixel:     m_RenderState.pixelResourceHandles[slot] = ((GenericBufferD3D12*)resource)->GetSrvHandle(); break;
            case RB::ShaderCompiler::Stage::kCompute:   m_RenderState.computeResourceHandles[slot] = ((GenericBufferD3D12*)resource)->GetSrvHandle(); break;
            default:
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Shader stage not recognized as shader resource input");
                break;
            }
        }
        break;



        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "This resource type is not yet supported as a shader input");
            break;
        }
    }

    void RenderInterfaceD3D12::SetRandomReadWriteInput(uint32_t handle, RenderResource* resource)
    {
        if (!resource->AllowedRandomReadWrites())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Resource cannot be set as random read/write input");
            return;
        }

        TransitionResource(resource, ResourceState::UNORDERED_ACCESS);

        uint32_t binding_offset = (handle >> 3) & 0x1FFFFFFF;
        ShaderCompiler::Stage stage = (ShaderCompiler::Stage)(handle & 0x7);

        // Descriptor handle takes up a uint2 in Slang
        uint32_t slot = binding_offset / sizeof(uint64_t);

        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, slot < _countof(m_RenderState.vertexResourceHandles), "UAV input slot out of range");

        switch (resource->GetType())
        {
        case RenderResourceType::Texture2D:
        {
            switch (stage)
            {
            case RB::ShaderCompiler::Stage::kVertex:    m_RenderState.vertexResourceHandles[slot] = ((Texture2DD3D12*)resource)->GetUavHandle(); break;
            case RB::ShaderCompiler::Stage::kPixel:     m_RenderState.pixelResourceHandles[slot] = ((Texture2DD3D12*)resource)->GetUavHandle(); break;
            case RB::ShaderCompiler::Stage::kCompute:   m_RenderState.computeResourceHandles[slot] = ((Texture2DD3D12*)resource)->GetUavHandle(); break;
            default:
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Shader stage not recognized as random read write input");
                break;
            }
        }
        break;

        case RenderResourceType::Texture2DArray:
        {
            switch (stage)
            {
            case RB::ShaderCompiler::Stage::kVertex:    m_RenderState.vertexResourceHandles[slot] = ((Texture2DArrayD3D12*)resource)->GetUavHandle(); break;
            case RB::ShaderCompiler::Stage::kPixel:     m_RenderState.pixelResourceHandles[slot] = ((Texture2DArrayD3D12*)resource)->GetUavHandle(); break;
            case RB::ShaderCompiler::Stage::kCompute:   m_RenderState.computeResourceHandles[slot] = ((Texture2DArrayD3D12*)resource)->GetUavHandle(); break;
            default:
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Shader stage not recognized as random read write input");
                break;
            }
        }
        break;

        case RenderResourceType::GenericBuffer:
        {
            switch (stage)
            {
            case RB::ShaderCompiler::Stage::kVertex:    m_RenderState.vertexResourceHandles[slot] = ((GenericBufferD3D12*)resource)->GetUavHandle(); break;
            case RB::ShaderCompiler::Stage::kPixel:     m_RenderState.pixelResourceHandles[slot] = ((GenericBufferD3D12*)resource)->GetUavHandle(); break;
            case RB::ShaderCompiler::Stage::kCompute:   m_RenderState.computeResourceHandles[slot] = ((GenericBufferD3D12*)resource)->GetUavHandle(); break;
            default:
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Shader stage not recognized as random read write input");
                break;
            }
        }
        break;

        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "This resource type is not yet supported as a UAV input");
            break;
        }
    }

    void RenderInterfaceD3D12::ClearResourceInputs()
    {
        for (int i = 0; i < _countof(m_RenderState.vertexResourceHandles); ++i)
        {
            m_RenderState.vertexResourceHandles[i] = DescriptorIndex{};
        }
        for (int i = 0; i < _countof(m_RenderState.pixelResourceHandles); ++i)
        {
            m_RenderState.pixelResourceHandles[i] = DescriptorIndex{};
        }
        for (int i = 0; i < _countof(m_RenderState.computeResourceHandles); ++i)
        {
            m_RenderState.computeResourceHandles[i] = DescriptorIndex{};
        }
    }

    void RenderInterfaceD3D12::SetConstantShaderData(uint32_t slot, const void* data, uint32_t data_size)
    {
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, slot < _countof(m_RenderState.cbvAddresses), "Up the amount of possible CBV addresses");

        UploadAllocation allocation = g_TransientCBVAllocator->Allocate(data_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        memcpy(allocation.cpuWriteAddress, data, data_size);

        m_RenderState.cbvAddresses[slot] = allocation.gpuAddress;
    }

    void RenderInterfaceD3D12::ClearConstantShaderData()
    {
        memset(m_RenderState.cbvAddresses, 0, _countof(m_RenderState.cbvAddresses) * sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
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

    void RenderInterfaceD3D12::SetComputeShader(uint32_t shader_index)
    {
        m_RenderState.csShader = shader_index;
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

        // TODO Add UAV clear if possible on the resource
        // (Will then also have to implement a non-shader visible SRV/UAV descriptor heap, or just do a clear in a compute shader?)

        if (tex->AllowedRenderTarget())
        {
            TransitionResource(resource, ResourceState::RENDER_TARGET);

            PendingClear clear = {};
            clear.renderTarget = true;
            clear.color        = color;

            if (tex->GetType() == RenderResourceType::Texture2D)
                clear.handle = ((Texture2DD3D12*)tex)->GetRenderTargetHandle();
            else if (tex->GetType() == RenderResourceType::Texture2DArray)
                clear.handle = ((Texture2DArrayD3D12*)tex)->GetRenderTargetHandle();
            else
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "ResourceType not yet supported for a clear");

            m_RenderState.pendingClears.push_back(clear);
        }
        else if (tex->AllowedDepthStencil())
        {
            TransitionResource(resource, ResourceState::DEPTH_WRITE);

            PendingClear clear = {};
            clear.renderTarget = false;
            clear.color        = color;

            if (tex->GetType() == RenderResourceType::Texture2D)
                clear.handle = ((Texture2DD3D12*)tex)->GetDepthStencilTargetHandle();
            else if (tex->GetType() == RenderResourceType::Texture2DArray)
                clear.handle = ((Texture2DArrayD3D12*)tex)->GetDepthStencilTargetHandle();
            else
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "ResourceType not yet supported for a clear");

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

        for (uint32_t i = 0; i < total_viewports; ++i)
        {
            sizes[i].TopLeftX   = viewports[i].left;
            sizes[i].TopLeftY   = viewports[i].top;
            sizes[i].Width      = viewports[i].width;
            sizes[i].Height     = viewports[i].height;
            sizes[i].MinDepth   = D3D12_MIN_DEPTH;
            sizes[i].MaxDepth   = D3D12_MAX_DEPTH;
        }

        m_CommandList->RSSetViewports(total_viewports, sizes);

        m_RenderState.viewportSet = true;
        m_RenderState.psoDirty = true;
    }

    void RenderInterfaceD3D12::SetScissor(const Viewport& scissor)
    {
        SetScissors(&scissor, 1);
    }

    void RenderInterfaceD3D12::SetScissors(const Viewport* scissors, uint32_t total_scissors)
    {
        D3D12_RECT* rects = ALLOC_STACKC(D3D12_RECT, total_scissors);

        for (uint32_t i = 0; i < total_scissors; ++i)
        {
            rects[i].left   = scissors[i].left;
            rects[i].right  = scissors[i].left + scissors[i].width;
            rects[i].top    = scissors[i].top;
            rects[i].bottom = scissors[i].top + scissors[i].height;
        }

        m_CommandList->RSSetScissorRects(total_scissors, rects);

        m_RenderState.scissorSet = true;
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

        uint32_t last_count = m_RenderState.vertexBufferCount;
        m_RenderState.vertexBufferCount = resource_count;

        D3D12_PRIMITIVE_TOPOLOGY_TYPE last_type = m_RenderState.vertexBufferType;

        switch (base_vbo->GetTopologyType())
        {
        case TopologyType::TriangleStrip:
        case TopologyType::TriangleList:	m_RenderState.vertexBufferType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;

        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Topology type not yet implemented");
            m_RenderState.vertexBufferType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
            break;
        }

        if (last_type != m_RenderState.vertexBufferType ||
            last_count != m_RenderState.vertexBufferCount)
        {
            m_RenderState.psoDirty = true;
        }
    }

    void RenderInterfaceD3D12::CopyResource(RenderResource* src, RenderResource* dst)
    {
        GpuResource* src_res = (GpuResource*)src->GetNativeResource();
        GpuResource* dst_res = (GpuResource*)dst->GetNativeResource();

        // We do not need to transition resources to the copy state if we are in COMMON because of implicit state promotion.
        if (!src_res->IsInState(D3D12_RESOURCE_STATE_COMMON))
            g_ResourceStateManager->TransitionResource(src_res, D3D12_RESOURCE_STATE_COPY_SOURCE);
        if (!dst_res->IsInState(D3D12_RESOURCE_STATE_COMMON))
            g_ResourceStateManager->TransitionResource(dst_res, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

        const RenderResourceType src_type = src->GetPrimitiveType();
        const RenderResourceType dst_type = dst->GetPrimitiveType();

        if (src_type == RenderResourceType::Buffer && dst_type == RenderResourceType::Buffer)
        {
            m_CommandList->CopyResource(dst_res->GetResource(), src_res->GetResource());
        }
        else if (src_type == RenderResourceType::Texture && dst_type == RenderResourceType::Buffer)
        {
            Texture* tex = (Texture*)src;
            D3D12_RESOURCE_DESC tex_desc = ((GpuResource*)tex->GetNativeResource())->GetResource()->GetDesc();

            uint32_t first_subresource = D3D12CalcSubresource(tex->GetBaseMip(), tex->GetFirstArraySlice(), 0, tex->GetMipCount(), tex->GetArraySize());
            uint32_t subresource_count = (tex->GetMipCount() * tex->GetArraySize()) - first_subresource;

            List<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(subresource_count);
            List<UINT> num_rows(subresource_count);
            List<UINT64> row_sizes(subresource_count);
            UINT64 totalBytes;
            g_GraphicsDevice->Get()->GetCopyableFootprints(&tex_desc,
                                                           first_subresource,
                                                           subresource_count,
                                                           0,
                                                           layouts.data(),
                                                           num_rows.data(),
                                                           row_sizes.data(),
                                                           &totalBytes);

            for (uint32_t i = 0; i < subresource_count; ++i)
            {
                D3D12_TEXTURE_COPY_LOCATION src_loc = {};
                src_loc.pResource           = src_res->GetResource();
                src_loc.Type                = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                src_loc.SubresourceIndex    = first_subresource + i;

                D3D12_TEXTURE_COPY_LOCATION dst_loc = {};
                dst_loc.pResource            = dst_res->GetResource();
                dst_loc.Type                 = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                dst_loc.PlacedFootprint      = layouts[i];

                m_CommandList->CopyTextureRegion(&dst_loc, 0, 0, 0, & src_loc, nullptr);
            }
        }
        else
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Copy step not yet implemented");
        }
    }

    void RenderInterfaceD3D12::Readback(RenderResource* src, ReadbackBuffer* dst)
    {
        CopyResource(src, dst);
        m_SchedulesReadbacks.push_back(dst);
    }

    void RenderInterfaceD3D12::UploadDataToResource(RenderResource* resource, const void* data, uint64_t data_size)
    {
        RB_ASSERT(LOGTAG_GRAPHICS, m_CopyOperationsOnly, "This operation should only be done on a Copy Queue!");

        GpuResource* gpu_res = (GpuResource*)resource->GetNativeResource();
        
        RB_ASSERT(LOGTAG_GRAPHICS, gpu_res->IsInState(D3D12_RESOURCE_STATE_COMMON) || gpu_res->IsInState(D3D12_RESOURCE_STATE_COPY_DEST), 
            "Resource is not in the correct state to upload data to");

        switch (resource->GetPrimitiveType())
        {
        case RenderResourceType::Buffer:
        {
            UploadAllocation upload_alloc = g_TransientUploadAllocator->Allocate(data_size);

            memcpy(upload_alloc.cpuWriteAddress, data, data_size);

            m_CommandList->CopyBufferRegion(gpu_res->GetResource(), 0, upload_alloc.resource->GetResource(), upload_alloc.offset, data_size);
        }
        break;

        case RenderResourceType::Texture:
        {
            D3D12_RESOURCE_DESC desc             = gpu_res->GetResource()->GetDesc();
            const bool          is_3d            = (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D);
            const uint32_t      array_size       = is_3d ? 1 : desc.DepthOrArraySize;
            const uint32_t      num_subresources = desc.MipLevels * array_size;

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts   = ALLOC_STACKC(D3D12_PLACED_SUBRESOURCE_FOOTPRINT, num_subresources);
            UINT*                               num_rows  = ALLOC_STACKC(UINT, num_subresources);
            UINT64*                             row_sizes = ALLOC_STACKC(UINT64, num_subresources);

            UINT64 total_bytes = 0;
            g_GraphicsDevice->Get()->GetCopyableFootprints(&desc, 0, num_subresources, 0, layouts, num_rows, row_sizes, &total_bytes);

            UploadAllocation upload_alloc = g_TransientUploadAllocator->Allocate(total_bytes, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

            uint8_t* dst_base = upload_alloc.cpuWriteAddress;
            uint8_t* src_cursor = (uint8_t*)data;

            const bool is_bc = IsBlockCompressedFormat(resource->GetFormat());
            const uint32_t bytes_per_block = is_bc ? GetBytesPerBlockFromFormat(resource->GetFormat()) : 0;
            const uint32_t bytes_per_pixel = is_bc ? 0 : GetElementSizeFromFormat(resource->GetFormat());

            for (uint32_t array_slice = 0; array_slice < array_size; ++array_slice)
            {
                for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
                {
                    const uint32_t sub = D3D12CalcSubresource(mip, array_slice, 0, desc.MipLevels, array_size);

                    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& fp = layouts[sub];
                    const UINT   rows           = num_rows[sub];
                    const UINT64 row_size_bytes = row_sizes[sub];
                    const UINT   depth          = fp.Footprint.Depth;

                    UINT w = Math::Max(1u, (UINT)desc.Width >> mip);
                    UINT h = Math::Max(1u, (UINT)desc.Height >> mip);

                    uint64_t src_row_pitch;
                    uint64_t src_slice_pitch;
                    if (is_bc)
                    {
                        UINT blocks_wide = Math::Max(1u, (w + 3) / 4);
                        UINT blocks_high = Math::Max(1u, (h + 3) / 4);
                        src_row_pitch   = blocks_wide * bytes_per_block;
                        src_slice_pitch = src_row_pitch * blocks_high;
                    }
                    else
                    {
                        src_row_pitch   = w * bytes_per_pixel;
                        src_slice_pitch = src_row_pitch * h;
                    }

                    uint8_t* dst_subresource_base = dst_base + fp.Offset;

                    for (UINT z = 0; z < depth; ++z)
                    {
                        uint8_t* src_slice = src_cursor + z * src_slice_pitch;
                        uint8_t* dst_slice = dst_subresource_base + z * fp.Footprint.RowPitch * rows;

                        for (UINT row = 0; row < rows; ++row)
                        {
                            memcpy(dst_slice + row * fp.Footprint.RowPitch,
                                   src_slice + row * src_row_pitch,
                                   row_size_bytes);
                        }
                    }

                    src_cursor += src_slice_pitch * depth;
                }
            }

            for (uint32_t array_slice = 0; array_slice < array_size; ++array_slice)
            {
                for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
                {
                    const uint32_t sub = D3D12CalcSubresource(mip, array_slice, 0, desc.MipLevels, array_size);

                    D3D12_TEXTURE_COPY_LOCATION src = {};
                    src.pResource               = upload_alloc.resource->GetResource();
                    src.Type                    = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                    src.PlacedFootprint         = layouts[sub];
                    src.PlacedFootprint.Offset += upload_alloc.offset;

                    D3D12_TEXTURE_COPY_LOCATION dst = {};
                    dst.pResource        = gpu_res->GetResource();
                    dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    dst.SubresourceIndex = sub;

                    m_CommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
                }
            }
        }
        break;

        default:
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Upload not possible for this type");
            break;
        }
    }

    void RenderInterfaceD3D12::PrepareDraw()
    {
        HandlePendingClears();
        FlushResourceBarriers();

        if (m_RenderState.renderTargetDirty)
        {
            SetRenderTargets();
        }

        if (m_RenderState.psoDirty || m_RenderState.rootSignatureDirty)
        {
            SetGraphicsPipelineState();
        }

        BindResources(false);
    }

    void RenderInterfaceD3D12::DrawInternal()
    {
        PrepareDraw();

        if (m_RenderState.indexCountPerInstance > 0)
        {
            m_CommandList->DrawIndexedInstanced(m_RenderState.indexCountPerInstance, 1, 0, 0, 0);
        }
        else
        {
            m_CommandList->DrawInstanced(m_RenderState.vertexCountPerInstance, 1, 0, 0);
        }
    }

    void RenderInterfaceD3D12::DrawInstancedInternal(uint32_t instances)
    {
        PrepareDraw();

        if (m_RenderState.indexCountPerInstance > 0)
        {
            m_CommandList->DrawIndexedInstanced(m_RenderState.indexCountPerInstance, 1, instances, 0, 0);
        }
        else
        {
            m_CommandList->DrawInstanced(m_RenderState.vertexCountPerInstance, 1, instances, 0);
        }
    }

    void RenderInterfaceD3D12::DispatchInternal(uint32_t thread_groups_x, uint32_t thread_groups_y, uint32_t thread_groups_z)
    {
        HandlePendingClears();
        FlushResourceBarriers();

        if (m_RenderState.psoDirty || m_RenderState.rootSignatureDirty)
        {
            SetComputePipelineState();
        }

        BindResources(true);

        m_CommandList->Dispatch(thread_groups_x, thread_groups_y, thread_groups_z);
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
        auto heaps = g_DescriptorManager->GetPipelineHeaps(num_heaps);
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
                m_CommandList->ClearDepthStencilView(clear.handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clear.color.r, clear.color.g, 0, nullptr);
            }
        }

        m_RenderState.pendingClears.clear();
    }

    void RenderInterfaceD3D12::SetRenderTargets()
    {
        m_RenderState.renderTargetDirty = false;

        D3D12_CPU_DESCRIPTOR_HANDLE handles[8];
        for (int i = 0; i < m_RenderState.numRenderTargets; i++)
        {
            handles[i] = m_RenderState.rtvHandles[i].top();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE* dsv_handle = nullptr;
        if (m_RenderState.dsvFormat != DXGI_FORMAT_UNKNOWN)
        {
            dsv_handle = &m_RenderState.dsvHandle;
        }

        m_CommandList->OMSetRenderTargets(m_RenderState.numRenderTargets, handles, false, dsv_handle);

        if (!m_RenderState.viewportSet || !m_RenderState.scissorSet)
        {
            Viewport vp;
            vp.left     = 0;
            vp.top      = 0;
            vp.width    = m_RenderState.width;
            vp.height   = m_RenderState.height;

            if (!m_RenderState.viewportSet)
                SetViewport(vp);
            if (!m_RenderState.scissorSet)
            SetScissor(vp);
        }
    }

    void RenderInterfaceD3D12::BindResources(bool compute)
    {
        uint32_t next_root_index = 0;
        for (int i = 0; i < 3; i++)
        {
            DescriptorIndex* handles;
            if (i == 0)
                handles = m_RenderState.vertexResourceHandles;
            else if (i == 1)
                handles = m_RenderState.pixelResourceHandles;
            else
                handles = m_RenderState.computeResourceHandles;

            // Set the bindless SRV/UAV slots in the root constants
            uint32_t values_to_set = 0;
            uint32_t* descriptor_handle_values = ALLOC_STACKC(uint32_t, _countof(m_RenderState.vertexResourceHandles) * 2);
            for (int i = 0; i < _countof(m_RenderState.vertexResourceHandles); i++)
            {
                if (handles[i].isValid())
                {
                    descriptor_handle_values[values_to_set++] = handles[i].heapIndex;   // Resource heap index
                    descriptor_handle_values[values_to_set++] = 0;                      // Sampler heap index (for Sampler2D)
                }
            }
            if (values_to_set > 0)
            {
                if (compute)
                    m_CommandList->SetComputeRoot32BitConstants(next_root_index, values_to_set, descriptor_handle_values, 0);
                else
                    m_CommandList->SetGraphicsRoot32BitConstants(next_root_index, values_to_set, descriptor_handle_values, 0);

                next_root_index++;
            }
        }

        uint32_t cbv_offset = 0;
        if (compute)
            cbv_offset = g_PipelineManager->GetRootSignatureCbvBindingOffset(m_RenderState.csShader);
        else
            cbv_offset = g_PipelineManager->GetRootSignatureCbvBindingOffset(m_RenderState.vsShader, m_RenderState.psShader);

        // Need to account for any CBV's that are in the root signature of this shader but not actually being used
        // (since Slang does not remove unused CBV's from the compiled code)
        uint32_t root_constants_offset = cbv_offset - next_root_index;

        // Bind the CBV's
        for (int i = 0; i < _countof(m_RenderState.cbvAddresses); ++i)
        {
            if (m_RenderState.cbvAddresses[i] > 0)
            {
                if (compute)
                    m_CommandList->SetComputeRootConstantBufferView(i - root_constants_offset, m_RenderState.cbvAddresses[i]);
                else
                    m_CommandList->SetGraphicsRootConstantBufferView(i - root_constants_offset, m_RenderState.cbvAddresses[i]);
            }
        }
    }

    void RenderInterfaceD3D12::SetGraphicsPipelineState()
    {
        #define CHECK_SET(check, message) if (!(check)) { RB_LOG_ERROR(LOGTAG_GRAPHICS, message); return; }

        CHECK_SET(m_RenderState.vertexBufferType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,    "Cannot draw, vertex buffer type was not set")
        CHECK_SET(m_RenderState.scissorSet,                                                     "Cannot draw, scissor was not set")
        CHECK_SET(m_RenderState.viewportSet,                                                    "Cannot draw, viewport was not set")
        CHECK_SET(m_RenderState.vsShader >= 0,                                                  "Cannot draw, vertex shader was not set yet")
        CHECK_SET(m_RenderState.blendingSet,                                                    "Cannot draw, blend mode was not set")
        CHECK_SET(m_RenderState.rasterizerSet,                                                  "Cannot draw, rasterizer was not set")
        CHECK_SET(m_RenderState.depthStencilSet,                                                "Cannot draw, depth stencil was not set")

        #undef CHECK_SET

        if (m_RenderState.rootSignatureDirty)
        {
            m_RenderState.rootSignature = g_PipelineManager->GetRootSignature(m_ShaderSystem, m_RenderState.vsShader, m_RenderState.psShader);

            m_RenderState.rootSignatureDirty = false;
        }

        const List<D3D12_INPUT_ELEMENT_DESC> input_elements = g_PipelineManager->GetInputElementDesc(m_ShaderSystem, m_RenderState.vsShader, m_RenderState.vertexBufferCount);

        const CompiledShaderBlob* vs_blob = m_ShaderSystem->GetCompiledShader(m_RenderState.vsShader);
        const CompiledShaderBlob* ps_blob = m_ShaderSystem->GetCompiledShader(m_RenderState.psShader);

        DXGI_FORMAT formats[8];
        for (int i = 0; i < 8; i++)
        {
            if (i < m_RenderState.numRenderTargets)
                formats[i] = m_RenderState.rtvFormats[i].top();
            else
                formats[i] = DXGI_FORMAT_UNKNOWN;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        pso_desc.pRootSignature         = m_RenderState.rootSignature.Get();
        pso_desc.VS                     = { vs_blob->blob, vs_blob->size };
        pso_desc.PS                     = { ps_blob ? ps_blob->blob : nullptr, ps_blob ? ps_blob->size : 0 };
        //pso_desc.DS                   = ;
        //pso_desc.HS                   = ;
        //pso_desc.GS                   = ;
        //pso_desc.StreamOutput         = ;
        pso_desc.BlendState             = m_RenderState.blendDesc;
        pso_desc.SampleMask             = UINT_MAX;
        pso_desc.RasterizerState        = m_RenderState.rasterizerDesc;
        pso_desc.DepthStencilState      = m_RenderState.depthStencilDesc;
        pso_desc.InputLayout            = { input_elements.data(), (UINT)input_elements.size() };
        //pso_desc.IBStripCutValue      = ;
        pso_desc.PrimitiveTopologyType  = m_RenderState.vertexBufferType;
        pso_desc.NumRenderTargets       = m_RenderState.numRenderTargets;
        /*pso_desc.RTVFormats */          memcpy(pso_desc.RTVFormats, formats, sizeof(DXGI_FORMAT) * 8);
        pso_desc.DSVFormat              = m_RenderState.dsvFormat;
        pso_desc.SampleDesc             = { 1, 0 };
        pso_desc.NodeMask               = 0;
        //pso_desc.CachedPSO            = NULL;
        pso_desc.Flags                  = D3D12_PIPELINE_STATE_FLAG_NONE;

        GPtr<ID3D12PipelineState> pso = g_PipelineManager->GetGraphicsPipeline(pso_desc, m_RenderState.vsShader, m_RenderState.psShader);

        m_CommandList->SetPipelineState(pso.Get());
        // All bound resources are not valid anymore after this
        m_CommandList->SetGraphicsRootSignature(m_RenderState.rootSignature.Get());

        m_RenderState.psoDirty = false;
    }
    
    void RenderInterfaceD3D12::SetComputePipelineState()
    {
        #define CHECK_SET(check, message) if (!(check)) { RB_LOG_ERROR(LOGTAG_GRAPHICS, message); return; }

        CHECK_SET(m_RenderState.csShader >= 0,      "Cannot Dispatch, compute shader was not set yet")

        #undef CHECK_SET

        if (m_RenderState.rootSignatureDirty)
        {
            m_RenderState.rootSignature = g_PipelineManager->GetRootSignature(m_ShaderSystem, m_RenderState.csShader);

            m_RenderState.rootSignatureDirty = false;
        }

        const CompiledShaderBlob* cs_blob = m_ShaderSystem->GetCompiledShader(m_RenderState.csShader);

        D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
        pso_desc.pRootSignature = m_RenderState.rootSignature.Get();
        pso_desc.CS             = { cs_blob->blob, cs_blob->size };
        pso_desc.NodeMask       = 0;
        //pso_desc.CachedPSO    = NULL;
        pso_desc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;

        GPtr<ID3D12PipelineState> pso = g_PipelineManager->GetComputePipeline(pso_desc, m_RenderState.csShader);

        m_CommandList->SetPipelineState(pso.Get());
        // All bound resources are not valid anymore after this
        m_CommandList->SetComputeRootSignature(m_RenderState.rootSignature.Get());

        m_RenderState.psoDirty = false;
    }
}
#endif