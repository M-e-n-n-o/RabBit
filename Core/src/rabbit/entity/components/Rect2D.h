#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "app/AssetManager.h"
#include "graphics/RenderResource.h"

namespace RB::Entity
{
    class Font
    {
    public:
        Font(const char* name, LoadedFont& loaded_font)
        {
            LoadedImage& img = loaded_font.fontAtlas;
            m_FontTexture = Graphics::Texture2D::Create(name, img.data, img.dataSize, img.format, img.width, img.height, false, false);

            m_Characters = loaded_font.characters;
        }

        ~Font()
        {
            SAFE_DELETE(m_FontTexture);
        }

        Graphics::Texture2D* GetFontTexture() const
        {
            return m_FontTexture;
        }

        const Map<char, LoadedFont::Character>* GetCharacterMap() const
        {
            return &m_Characters;
        }

    private:
        Graphics::Texture2D* m_FontTexture;
        Map<char, LoadedFont::Character> m_Characters;
    };

    class Rect2D : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("Rect2D");

        Rect2D(float width, float height)
            : width(width)
            , height(height)
        {}

        float width;
        float height;
    };

    class Text2D : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("Text2D");

        Text2D(Font* font, const std::string& text, float w = 0, float h = 0)
            : font(font)
            , text(text)
            , width(w)
            , height(h)
        {
            textBoundWidth = 0;
            textBoundHeight = 0;
            textMaxBearingUpper = 0;
            float textMaxBearingLower = 0;
            auto* cm = font->GetCharacterMap();
            for (char c : text)
            {
                const auto& ch = cm->at(c);
                textBoundWidth += ch.advance;
                textMaxBearingLower = Math::Max(ch.size.y - ch.bearing.y, textMaxBearingLower);
                textMaxBearingUpper = Math::Max(ch.bearing.y, textMaxBearingUpper);
            }

            textBoundHeight = textMaxBearingUpper + textMaxBearingLower;

            if (width == 0)
                width = textBoundWidth;
            if (height == 0)
                height = textBoundHeight;
        }

        Font* font;
        std::string text;
        float width;
        float height;

        float textBoundWidth;
        float textBoundHeight;
        float textMaxBearingUpper;
    };
}