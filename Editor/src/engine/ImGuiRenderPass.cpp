#include "RabBitCommon.h"
#include "ImGuiRenderPass.h"
#include "EngineEditorWindow.h"
#include "ImGuiManager.h"

#include "app/Application.h"

#include "entity/Scene.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "imgui.h"
#include "imgui_threaded_rendering.h"
#include "backends/imgui_impl_dx12.h"

using namespace RB;
using namespace RB::Graphics;
using namespace RB::Entity;

namespace Editor
{
    struct ImGuiRenderEntry : public RenderPassEntry
    {
        ImGuiContext* context;
        ImDrawDataSnapshot* snapshot;
    };

    RenderPassConfig ImGuiRenderPass::GetConfiguration(const RenderPassSettings& setting)
    {
        return RenderPassConfig
        {
            // Dependencies
            {
                RenderTextureInputDesc{"Color", 0}
            },

            // Working textures
            {},

            // Output textures
            {
                RenderResourceDesc {
                    .name     = "ColorOverlay",
                    .format   = RenderResourceFormat::RGBA32_FLOAT,
                    .type     = RenderResourcePassType::Tex2D,
                    .typeDesc = { kRTSize_Full, kRTSize_Full, 1 },
                    .flags    = kRTFlag_AllowRenderTarget
                }
            },

            // Async compute compatible
            false
        };
    }

    RenderPassEntry* ImGuiRenderPass::SubmitEntry(const ViewContext* view_context, const Scene* const scene, FrameAllocator* allocator)
    {
        if (view_context->isOffscreen)
        {
            RB_LOG_WARN("ImGuiRenderer is not supported for offscreen contexts");
            return nullptr;
        }

        const auto& list = scene->GetComponentsWithTypeOf<ImGuiManager>();
        if (list.empty())
        {
            RB_LOG_WARN("ImGuiRenderer requires a ImGuiManager to work!");
            return nullptr;
        }

        ImGuiRenderEntry* entry = (ImGuiRenderEntry*)allocator->Allocate(sizeof(ImGuiRenderEntry));
        memset(entry, 0, sizeof(ImGuiRenderEntry));

        auto* window = dynamic_cast<EngineEditorWindow*>(Application::GetInstance()->GetWindow(view_context->windowIndex));
        window->Select();

        // Prepare render data for rendering
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        ImGuiContext* original_context = ImGui::GetCurrentContext();

        // Set the render thread context
        entry->context = ((ImGuiManager*)list[0])->GetRenderContext();
        ImGui::SetCurrentContext(entry->context);

        // Create a snapshot of the render data for the RenderThread
        SetCurrentThreadImGuiAllocatorMode(AllocatorMode::RenderTransient);
        entry->snapshot = (ImDrawDataSnapshot*)allocator->Allocate(sizeof(ImDrawDataSnapshot));
        entry->snapshot->SnapUsingCopy(draw_data, ImGui::GetTime());
        SetCurrentThreadImGuiAllocatorMode(AllocatorMode::Persistent);

        // Restore the main thread context
        ImGui::SetCurrentContext(original_context);

        return entry;
    }

    void ImGuiRenderPass::Render(RenderPassInput& in)
    {
        ImGuiRenderEntry* entry = (ImGuiRenderEntry*)in.entryContext;

        in.ri->PushRenderTarget(in.outputRes[0]);
        in.ri->FlushAllPending();

        ImGui::SetCurrentContext(entry->context);
        // ImGui is only allocating textures in the following calls, so we should use persistent memory
        SetCurrentThreadImGuiAllocatorMode(AllocatorMode::Persistent);

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplDX12_RenderDrawData(&entry->snapshot->DrawData, (ID3D12GraphicsCommandList*)in.ri->GetNativeInterface());
    }
}