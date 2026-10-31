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
        , m_LeftMagnet(false)
        , m_RightMagnet(false)
        , m_TopMagnet(false)
        , m_BottomMagnet(false)
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

    void UIBox::AddConstraint(UIConstraintType type)
    {
        switch (type)
        {
        case UIConstraintType::Left:  m_LeftMagnet = true; break;
        case UIConstraintType::Right: m_RightMagnet = true; break;
        case UIConstraintType::Up:    m_TopMagnet = true; break;
        case UIConstraintType::Down:  m_BottomMagnet = true; break;
        default:
            break;
        }
    }

    // Pixels per 1 unit on each axis
    static Pair<float, float> UnitScale(UIUnit unit, float parent_w, float parent_h)
    {
        switch (unit)
        {
        case UIUnit::PCT:  return MakePair(parent_w / 100.0f, parent_h / 100.0f);
        case UIUnit::IPCT:
        {
            float s = Math::Min(parent_w, parent_h) / 100.0f;
            return MakePair(s, s);
        }
        default: return MakePair(1.0f, 1.0f);
        }
    }

    Pair<float, float> UIBox::ConvertUnits(UIUnit current, UIUnit target, float x, float y, float parent_w, float parent_h)
    {
        if (current == target)
            return MakePair(x, y);

        auto [cx, cy] = UnitScale(current, parent_w, parent_h);
        auto [tx, ty] = UnitScale(target, parent_w, parent_h);

        // A zero-sized parent can't express anything in relative units
        return MakePair(tx != 0.0f ? x * cx / tx : 0.0f, ty != 0.0f ? y * cy / ty : 0.0f);
    }

    void UIBox::GetParentSize(float& width, float& height) const
    {
        width = 0;
        height = 0;

        GameObject* parent = m_GameObject->GetParent();
        if (parent == nullptr)
            return;

        if (auto* canvas = parent->GetComponent<UICanvas>())
        {
            if (Camera* cam = canvas->GetTargetCamera())
            {
                width  = (float)cam->GetRenderTargetWidth();
                height = (float)cam->GetRenderTargetHeight();
            }
        }
        else if (auto* box = parent->GetComponent<UIBox>())
        {
            auto [size_x, size_y] = box->GetSize(UIUnit::PX);
            width = size_x;
            height = size_y;
        }
    }

    void UIBox::ResolvePx(float& x, float& y, float& w, float& h) const
    {
        float pw, ph;
        GetParentSize(pw, ph);

        std::tie(x, y) = ConvertUnits(m_PosUnit, UIUnit::PX, m_PosX, m_PosY, pw, ph);
        std::tie(w, h) = ConvertUnits(m_SizeUnit, UIUnit::PX, m_Width, m_Height, pw, ph);

        // One uniform padding in pixels (taken from the X axis so PCT padding is the same on both axes)
        float pad = ConvertUnits(m_PaddingUnit, UIUnit::PX, m_Padding, m_Padding, pw, ph).first;

        if (m_LeftMagnet && m_RightMagnet)
            x = (pw - w) / 2.0f;
        else if (m_LeftMagnet)
            x = pad;
        else if (m_RightMagnet)
            x = pw - w - pad;

        if (m_TopMagnet && m_BottomMagnet)
            y = (ph - h) / 2.0f;
        else if (m_TopMagnet)
            y = pad;
        else if (m_BottomMagnet)
            y = ph - h - pad;
    }

    Pair<float, float> UIBox::GetStartPos(UIUnit unit) const
    {
        float x, y, w, h, pw, ph;
        ResolvePx(x, y, w, h);
        GetParentSize(pw, ph);
        return ConvertUnits(UIUnit::PX, unit, x, y, pw, ph);
    }

    Pair<float, float> UIBox::GetSize(UIUnit unit) const
    {
        float x, y, w, h, pw, ph;
        ResolvePx(x, y, w, h);
        GetParentSize(pw, ph);
        return ConvertUnits(UIUnit::PX, unit, w, h, pw, ph);
    }

    Pair<float, float> UIBox::GetWorldStartPos() const
    {
        auto [x, y] = GetStartPos(UIUnit::PX);

        // Start positions are local to the parent, so accumulate them up the chain
        GameObject* parent_obj = GetGameObject()->GetParent();
        while (parent_obj)
        {
            const UIBox* parent_box = parent_obj->GetComponent<UIBox>();
            if (parent_box == nullptr)
                break;

            auto [start_x, start_y] = parent_box->GetStartPos(UIUnit::PX);
            x += start_x;
            y += start_y;

            parent_obj = parent_obj->GetParent();
        }

        return MakePair(x, y);
    }

    Pair<float, float> UIBox::GetWorldSize() const
    {
        // Size should not be accumulated against parents
        return GetSize(UIUnit::PX);
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

    void ListView::OnUpdate(float delta_time)
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

        // Child positions are local to this list, so start at its origin
        float start_x = 0.0f;
        float start_y = 0.0f;

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

    Text2D::Text2D(Font* font, const std::string& text, float scale, int render_order)
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
            auto it = cm->find(c);
            if (it == cm->end())
                continue; // Glyph not in the font atlas
            const auto& ch = it->second;
            m_TextBoundsWidth += ch.advance;
            textMaxBearingLower = Math::Max(ch.size.y - ch.bearing.y, textMaxBearingLower);
            m_TextMaxBearingUpper = Math::Max(ch.bearing.y, m_TextMaxBearingUpper);
        }

        m_TextBoundsHeight = m_TextMaxBearingUpper + textMaxBearingLower;
    }
}