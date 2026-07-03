#pragma once

#include "WindowPanel.h"
#include <RabBit.h>

namespace Editor
{
    class HierarchyPanel : public WindowPanel
    {
    public:
        HierarchyPanel();

        void SetRoot(RB::Entity::GameObject* root);
        void SetRoot(RB::Entity::Scene* root);

        void OnCreate() override;
        void OnDestroy() override;
        void OnUpdate() override;

    private:
        void DrawNode(RB::Entity::GameObject* current);

        RB::Entity::Scene*      m_SceneRoot;
        RB::Entity::GameObject* m_Root;
        RB::Entity::GameObject* m_Selected;
    };
}