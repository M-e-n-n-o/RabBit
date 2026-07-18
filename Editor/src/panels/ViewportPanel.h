#pragma once

#include <RabBit.h>
#include "WindowPanel.h"

namespace Editor
{
    class ViewportPanel : public WindowPanel
    {
    public:
        ViewportPanel();

        void OnCreate() override;
        void OnDestroy() override;
        void OnUpdate() override;

        const RB::Shared<RB::Graphics::Texture2D>& GetSceneTexture() const { return m_SceneTexture; }

    private:
        RB::Shared<RB::Graphics::Texture2D> m_SceneTexture;
        float m_Width;
        float m_Height;

        static constexpr int c_HistoryLength = 500;
        uint32_t m_DeltaIndex;
        float m_MainThreadAverages[c_HistoryLength];
        float m_RenderThreadAverages[c_HistoryLength];
    };
}