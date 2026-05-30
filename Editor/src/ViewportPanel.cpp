#include "ViewportPanel.h"

// We need these defines to be able to access the platform files inside RabBit
#define RB_GRAPHICS_API_D3D12 1
#define RB_PLATFORM_WINDOWS 1
#include <RabBit.h>
#include <rabbit/events/Event.h>
#include <rabbit/events/ApplicationEvent.h>

#include "platform/graphics/d3d12/resource/RenderResourceD3D12.h"
#include "platform/graphics/d3d12/resource/Descriptor.h"

using namespace RB;
using namespace RB::Graphics;

namespace Editor
{
    ViewportPanel::ViewportPanel()
        : m_Width(1280)
        , m_Height(720)
    {
        m_SceneTexture = Texture2D::Create("Game scene", RenderResourceFormat::R8G8B8A8_UNORM, m_Width, m_Height, true, true);
    }

    void ViewportPanel::OnCreate()
    {

    }

    void ViewportPanel::OnDestroy()
    {

    }

    void ViewportPanel::OnUpdate()
    {
        ImGui::Begin("Viewport");
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(D3D12::g_DescriptorManager->GetGpuHandle((std::static_pointer_cast<D3D12::Texture2DD3D12>(m_SceneTexture)->GetSrvHandle())).ptr), ImVec2(m_Width, m_Height));

        if ((Math::Abs(size.x - m_Width) > 0.01f || Math::Abs(size.y - m_Height) > 0.01f) &&
            size.x > 1.0f && size.y > 1.0f)
        {
            m_Width = size.x;
            m_Height = size.y;

            Events::RenderOutputChangedEvent e;
            Events::g_EventManager->InsertEvent(e);

            // Recreate the texture with the new size
            m_SceneTexture = Texture2D::Create("Game scene", RenderResourceFormat::R8G8B8A8_UNORM, m_Width, m_Height, true, true);
        }

        ImGui::End();
    }
}