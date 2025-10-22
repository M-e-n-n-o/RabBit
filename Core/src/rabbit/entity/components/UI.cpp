#include "RabBitCommon.h"
#include "UI.h"

namespace RB::Entity
{
    Text2D::Text2D(Font* font, const std::string& text, float scale, float width, float height, float render_order)
        : UIComponent(render_order)
        , m_Font(font)
        , m_Width(width)
        , m_Height(height)
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

        // If no size specified, just use the size of the entire text
        if (m_Width < 0)
            m_Width = m_TextBoundsWidth;
        if (m_Height < 0)
            m_Height = m_TextBoundsHeight;
    }
}