#include "RabBitCommon.h"
#include "UI.h"
#include "app/Application.h"
#include "graphics/Window.h"
#include "entity/GameObject.h"

namespace RB::Entity
{
    ListView::ListView(bool vertical, uint32_t padding)
        : m_Vertical(vertical)
        , m_Padding(padding)
        , m_LastWindowWidth(0)
        , m_LastWindowHeight(0)
    {
    }

    void ListView::Update()
    {
        auto* window = Application::GetInstance()->GetWindow(0);
        float width  = window->GetVirtualWidth();
        float height = window->GetVirtualHeight();

        if (m_LastWindowWidth != width || m_LastWindowHeight != height)
        {
            m_LastWindowWidth = width;
            m_LastWindowHeight = height;
            UpdateItemPositions();
        }
    }

    void ListView::OnChildAttached(GameObject* obj)
    {
        UpdateItemPositions();
    }

    void ListView::OnChildDettached(GameObject* obj)
    {
        UpdateItemPositions();
    }

    void ListView::UpdateItemPositions()
    {
        const auto* transform = m_GameObject->GetComponent<Transform>();
        const auto& children = m_GameObject->GetChildren();

        uint32_t start_x = transform->position.x;
        uint32_t start_y = transform->position.x;

        for (const auto& child : children)
        {
            // Check all UI render components in this child and calculate the total bounding box
            Math::Float2 biggest_bounds;
            uint32_t comp_idx = 0;
            while (true)
            {
                const auto* render_comp = child->GetComponent<UIRenderComponent>(comp_idx);
                if (render_comp == nullptr)
                    break;

                const Math::Float2& bounds = render_comp->GetBounds();
                biggest_bounds.x = Math::Max(bounds.x, biggest_bounds.x);
                biggest_bounds.y = Math::Max(bounds.x, biggest_bounds.y);
                ++comp_idx;
            }

            // Found any UIRenderComponents?
            if (comp_idx == 0)
                continue;

            auto* child_transform = child->GetComponent<Transform>();

            child_transform->position.x = start_x;
            child_transform->position.y = start_y;

            if (m_Vertical)
                start_y += biggest_bounds.y + m_Padding;
            else
                start_x += biggest_bounds.x + m_Padding;
        }
    }

    Text2D::Text2D(Font* font, const std::string& text, float scale, float width, float height, float render_order)
        : UIRenderComponent(render_order)
        , m_Font(font)
        , m_Scale(scale)
    {
        m_Bounds = Math::Float2(width, height);
        UpdateText(text);
    }

    void Text2D::UpdateText(const std::string& text)
    {
        m_Text = text;
        m_TextBoundsWidth = 0;
        m_TextMaxBearingUpper = 0;

        float textMaxBearingLower = 0;

        auto* cm = m_Font->GetCharacterMap();
        for (char c : text)
        {
            const auto& ch = cm->at(c);
            m_TextBoundsWidth += ch.advance;
            textMaxBearingLower = Math::Max(ch.size.y - ch.bearing.y, textMaxBearingLower);
            m_TextMaxBearingUpper = Math::Max(ch.bearing.y, m_TextMaxBearingUpper);
        }

        m_TextBoundsHeight = m_TextMaxBearingUpper + textMaxBearingLower;

        // If no size specified, just use the size of the entire text
        if (m_Bounds.x < 0)
            m_Bounds.x = m_TextBoundsWidth;
        if (m_Bounds.y < 0)
            m_Bounds.y = m_TextBoundsHeight;
    }
}