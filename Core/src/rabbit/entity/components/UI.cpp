#include "RabBitCommon.h"
#include "UI.h"
#include "app/Application.h"
#include "graphics/Window.h"
#include "entity/GameObject.h"
#include "entity/components/Camera.h"

namespace RB::Entity
{
    // ---------------------------------------------------------------------------
    //                                    UIBox
    // ---------------------------------------------------------------------------

    UIBox::UIBox()
        : m_PosUnit(UIUnit::PX)
        , m_PosX(0)
        , m_PosY(0)
        , m_SizeUnit(UIUnit::PX)
        , m_Width(0)
        , m_Height(0)
        , m_PaddingUnit(UIUnit::PX)
        , m_Padding(0)
        , m_LeftMagnet(0)
        , m_RightMagnet(0)
        , m_TopMagnet(0)
        , m_BottomMagnet(0)
        , m_LastParentWidth(0)
        , m_LastParentHeight(0)
    {
    }

    void UIBox::SetStartPos(UIUnit unit, float x, float y)
    {
        m_PosUnit = unit;
        m_PosX = x;
        m_PosY = y;
    }

    void UIBox::SetSize(UIUnit unit, float width, float height)
    {
        m_SizeUnit = unit;
        m_Width = width;
        m_Height = height;
    }

    void UIBox::SetPadding(UIUnit unit, float padding)
    {
        m_PaddingUnit = unit;
        m_Padding = padding;
    }

    Pair<float, float> UIBox::ConvertUnits(UIUnit current, UIUnit target, float x, float y) const
    {
        if (current == target)
        {
            return MakePair(x, y);
        }

        float parent_x, parent_y, parent_w, parent_h;
        GetParentBounds(parent_x, parent_y, parent_w, parent_h);

        if (target == UIUnit::PX)
        {
            if (current == UIUnit::PCT)
                return MakePair(x / 100.0f * parent_w, y / 100.0f * parent_h);
            else if (current == UIUnit::IPCT)
            {
                float smallest = Math::Min(parent_w, parent_h);
                return MakePair(x / 100.0f * smallest, y / 100.0f * smallest);
            }
        }
        else if (target == UIUnit::PCT)
        {
            if (current == UIUnit::PX)
                return MakePair(x / parent_w * 100.0f, y / parent_h * 100.0f);
            else if (current == UIUnit::IPCT)
            {
                float smallest = Math::Min(parent_w, parent_h);
                return MakePair(x * smallest / parent_w, y * smallest / parent_h);
            }
        }
        else if (target == UIUnit::IPCT)
        {
            float smallest = Math::Min(parent_w, parent_h);

            if (current == UIUnit::PX)
                return MakePair(x / smallest * 100.0f, y / smallest * 100.0f);
            else if (current == UIUnit::PCT)
                return MakePair(x * parent_w / smallest, y * parent_h / smallest);
        }

        RB_LOG_ERROR(LOGTAG_ENTITY, "Unit conversion not yet implemented");
        return MakePair(0, 0);
    }

    Pair<float, float> UIBox::GetStartPos(UIUnit unit) const
    {
        return ConvertUnits(m_PosUnit, unit, m_PosX, m_PosY);
    }

    Pair<float, float> UIBox::GetSize(UIUnit unit) const
    {
        return ConvertUnits(m_SizeUnit, unit, m_Width, m_Height);
    }

    void UIBox::Update()
    {
        float parent_x, parent_y, parent_w, parent_h;
        GetParentBounds(parent_x, parent_y, parent_w, parent_h);

        if (Math::Abs(m_LastParentWidth - parent_w) > 0.01f || Math::Abs(m_LastParentHeight - parent_h) > 0.01f)
        {
            m_LastParentWidth = parent_w;
            m_LastParentHeight = parent_h;
            UpdateMagnets();
        }
    }

    Pair<float, float> UIBox::GetWorldStartPos() const
    {
        auto [x, y] = GetStartPos(UIUnit::PX);
    
        GameObject* parent_obj = GetGameObject()->GetParent();
        while (parent_obj)
        {
            const UIBox* parent_box = parent_obj->GetComponent<UIBox>();
            if (parent_box == nullptr)
            {
                break;
            }
    
            auto [start_x, start_y] = parent_box->GetStartPos(UIUnit::PX);
            x += start_x;
            y += start_y;
    
            parent_obj = parent_obj->GetParent();
        }
    
        return MakePair(x, y);
    }

    Pair<float, float> UIBox::GetWorldSize() const
    {
        auto [width, height] = GetSize(UIUnit::PX);
    
        GameObject* parent_obj = GetGameObject()->GetParent();
        while (parent_obj)
        {
            const UIBox* parent_box = parent_obj->GetComponent<UIBox>();
            if (parent_box == nullptr)
            {
                break;
            }
    
            auto [size_x, size_y] = parent_box->GetSize(UIUnit::PX);
            width += size_x;
            height += size_y;
    
            parent_obj = parent_obj->GetParent();
        }
    
        return MakePair(width, height);
    }

    void UIBox::AddConstraint(UIConstraintType type)
    {
        switch (type)
        {
        case RB::Entity::UIConstraintType::Left:  m_LeftMagnet = true; break;
        case RB::Entity::UIConstraintType::Right: m_RightMagnet = true; break;
        case RB::Entity::UIConstraintType::Up:    m_TopMagnet = true; break;
        case RB::Entity::UIConstraintType::Down:  m_BottomMagnet = true; break;
        default:
            break;
        }
    }

    void UIBox::UpdateMagnets()
    {
        m_PosUnit = UIUnit::PX;
        auto [pad_x, pad_y] = ConvertUnits(m_PaddingUnit, m_PosUnit, m_Padding, m_Padding);
        auto [size_x, size_y] = GetSize(m_PosUnit);

        if (m_LeftMagnet && m_RightMagnet)
        {
            m_PosX = (m_LastParentWidth / 2.0f) - (size_x / 2.0f);
        }
        else if (m_LeftMagnet)
        {
            m_PosX = pad_x;
        }
        else if (m_RightMagnet)
        {
            m_PosX = m_LastParentWidth - size_x - pad_x;
        }

        if (m_TopMagnet && m_BottomMagnet)
        {
            m_PosY = (m_LastParentHeight / 2.0f) - (size_y / 2.0f);
        }
        else if (m_TopMagnet)
        {
            m_PosY = pad_y;
        }
        else if (m_BottomMagnet)
        {
            m_PosY = m_LastParentHeight - size_y - pad_y;
        }
    }

    void UIBox::GetParentBounds(float& x, float& y, float& width, float& height) const
    {
        GameObject* parent = m_GameObject->GetParent();
        if (parent == nullptr)
        {
            RB_LOG_WARN(LOGTAG_ENTITY, "A UIBox should always have a parent!");
            return;
        }

        if (auto* canvas = parent->GetComponent<UICanvas>())
        {
            if (canvas->GetTargetCamera() == nullptr)
            {
                RB_LOG_WARN(LOGTAG_ENTITY, "Canvas doesn't have a valid camera");
                return;
            }

            x = 0;
            y = 0;
            width  = canvas->GetTargetCamera()->GetRenderTargetWidth();
            height = canvas->GetTargetCamera()->GetRenderTargetHeight();
        }
        else if (auto* box = parent->GetComponent<UIBox>())
        {
            auto [start_x, start_y] = box->GetStartPos(UIUnit::PX);
            auto [size_x, size_y] = box->GetSize(UIUnit::PX);

            x = start_x;
            y = start_y;
            width = size_x;
            height = size_y;
        }
        else
        {
            RB_LOG_WARN(LOGTAG_ENTITY, "A UI box should have a UIBox or UICanvas parent!");

            x = 0;
            y = 0;
            width = 0;
            height = 0;
        }
    }

    // ---------------------------------------------------------------------------
    //                                  ListView
    // ---------------------------------------------------------------------------

    ListView::ListView(bool vertical, float element_padding)
        : m_Vertical(vertical)
        , m_ElementPadding(element_padding)
        , m_Box(nullptr)
    {
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
        if (m_Box == nullptr)
        {
            m_Box = m_GameObject->GetComponent<UIBox>();
            RB_ASSERT(LOGTAG_ENTITY, m_Box, "Every UI object (also ListView) should have a UIBox");
        }

        const auto& children = m_GameObject->GetChildren();

        auto [start_x, start_y] = m_Box->GetStartPos(UIUnit::PX);

        for (GameObject* child : children)
        {
            UIBox* child_box = child->GetComponent<UIBox>();

            if (child_box == nullptr)
            {
                RB_LOG_WARN(LOGTAG_ENTITY, "A UI object should have a UIBox component!");
                continue;
            }

            auto [bounds_x, bounds_y] = child_box->GetSize(UIUnit::PX);

            child_box->SetStartPos(UIUnit::PX, start_x, start_y);

            if (m_Vertical)
                start_y += bounds_y + m_ElementPadding;
            else
                start_x += bounds_x + m_ElementPadding;
        }
    }

    // ---------------------------------------------------------------------------
    //                                  Text2D
    // ---------------------------------------------------------------------------

    Text2D::Text2D(Font* font, const std::string& text, float scale, float render_order)
        : UIRenderComponent(render_order)
        , m_Font(font)
        , m_Scale(scale)
    {
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
    }
}