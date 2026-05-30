#pragma once

#include <RabBit.h>
#include "WindowPanel.h"

namespace Editor
{
    class ViewportPanel : public WindowPanel
    {
    public:
        ViewportPanel();

        virtual void OnCreate() override;
        virtual void OnDestroy() override;
        virtual void OnUpdate() override;

        RB::Shared<RB::Graphics::Texture2D> GetSceneTexture() const { return m_SceneTexture; }

    private:
        RB::Shared<RB::Graphics::Texture2D> m_SceneTexture;
        float m_Width;
        float m_Height;
    };
}