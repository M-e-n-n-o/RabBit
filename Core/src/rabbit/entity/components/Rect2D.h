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

        Text2D(Font* font, const std::string& text, float width, float height)
            : font(font)
            , text(text)
            , width(width)
            , height(height)
        {
        }

        Font* font;
        std::string text;
        float width;
        float height;
    };
}