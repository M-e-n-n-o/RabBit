#include "HierarchyPanel.h"
#include "imgui.h"

using namespace RB::Entity;

namespace Editor
{
    HierarchyPanel::HierarchyPanel()
        : m_SceneRoot(nullptr)
        , m_Root(nullptr)
        , m_Selected(nullptr)
    {
    }

    void HierarchyPanel::SetRoot(GameObject* root)
    {
        m_SceneRoot = nullptr;
        m_Root = root;
    }

    void HierarchyPanel::SetRoot(Scene* root)
    {
        m_SceneRoot = root;
        m_Root = nullptr;
    }

    void HierarchyPanel::OnCreate()
    {
    }

    void HierarchyPanel::OnDestroy()
    {
    }

    void HierarchyPanel::OnUpdate()
    {
        if (m_SceneRoot == nullptr && m_Root == nullptr)
            return;

        ImGui::Begin("Hierarchy View");

        if (m_SceneRoot)
        {
            for (const auto& objs : m_SceneRoot->GetGameObjects())
            {
                if (objs->GetParent() == nullptr)
                    DrawNode(objs);
            }
        }
        else
        {
            DrawNode(m_Root);
        }

        ImGui::End();
    }

    void HierarchyPanel::DrawNode(GameObject* current)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (m_Selected == current)
            flags |= ImGuiTreeNodeFlags_Selected;

        if (current->GetChildren().empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx(current, flags, "%s", current->GetName());

        if (ImGui::IsItemClicked())
            m_Selected = current;

        if (opened)
        {
            for (const auto& child : current->GetChildren())
            {
                DrawNode(child);
            }

            ImGui::TreePop();
        }
    }
}