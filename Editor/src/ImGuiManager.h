#pragma once

// We need these defines to be able to access the platform files inside RabBit
#define RB_GRAPHICS_API_D3D12 1
#define RB_PLATFORM_WINDOWS 1

#include "rabbit/entity/ObjectComponent.h"
#include "rabbit/graphics/RenderResource.h"
#include "imgui.h"

namespace Editor
{
    class ImGuiManager : public RB::Entity::ObjectComponent
    {
    public:
        ImGuiManager(ImGuiContext* render_context)
            : m_RenderContext(render_context)
        {
        }

        ImGuiContext* GetRenderContext() const { return m_RenderContext; }

    private:
        ImGuiContext* m_RenderContext;
    };

    // -----------------------------------
    // Global ImGui functions

    void InitializeImGui();

    ImGuiContext* CreateImGuiContext();
    void InitializeImGuiContextRenderBackend(ImGuiContext* ctx, RB::Graphics::RenderResourceFormat format);
    void DestroyImGuiContext(ImGuiContext* ctx);

    enum class AllocatorMode
    {
        Persistent,
        RenderTransient
    };

    void SetCurrentThreadImGuiAllocatorMode(AllocatorMode mode);
}