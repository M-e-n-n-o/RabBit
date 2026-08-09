#include "RabBitCommon.h"
#include "Camera.h"
#include "app/Application.h"
#include "graphics/Window.h"

namespace RB::Entity
{
    uint32_t Camera::GetRenderTargetWidth() const
    {
        if (m_TargetWindowHandle != nullptr)
        {
            auto* window = Application::GetInstance()->FindWindow(m_TargetWindowHandle);
            if (window == nullptr)
            {
                return 0;
            }

            return window->GetVirtualWidth();
        }

        return m_RenderTexture->GetWidth();
    }

    uint32_t Camera::GetRenderTargetHeight() const
    {
        if (m_TargetWindowHandle != nullptr)
        {
            auto* window = Application::GetInstance()->FindWindow(m_TargetWindowHandle);
            if (window == nullptr)
            {
                return 0;
            }

            return window->GetVirtualHeight();
        }

        return m_RenderTexture->GetHeight();
    }
}