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

        Graphics::Texture2D* GetFontTexture() const { return m_FontTexture; }

        const Map<char, LoadedFont::Character>* GetCharacterMap() const { return &m_Characters; }

    private:
        Graphics::Texture2D* m_FontTexture;
        Map<char, LoadedFont::Character> m_Characters;
    };

    // ---------------------------------------------------------------------------
    //                             UI Components
    // ---------------------------------------------------------------------------

    class UIComponent : public ObjectComponent
    {
    public:
        UIComponent(uint32_t render_order = 0)
            : m_RenderOrder(render_order)
        {
        }

        int GetRenderOrder() const { return m_RenderOrder; }

    private:
        uint32_t m_RenderOrder;
    };

    class Rect2D : public UIComponent
    {
    public:
        DEFINE_COMP_TAG("Rect2D");

        Rect2D(float width, float height, Math::Float4 color, float render_order = 0)
            : UIComponent(render_order)
            , width(width)
            , height(height)
            , color(color)
        {}

        float width;
        float height;
        Math::Float4 color;
    };

    class Text2D : public UIComponent
    {
    public:
        DEFINE_COMP_TAG("Text2D");

        Text2D(Font* font, const std::string& text, float scale = 1.0f, float width = -1, float height = -1, float render_order = 0);

        void UpdateText(const std::string& text);

        const Font* GetFont()   const { return m_Font; }
        std::string GetText()   const { return m_Text; }
        float       GetWidth()  const { return m_Width; }
        float       GetHeight() const { return m_Height; }
        float       GetScale()  const { return m_Scale; }
        
        float       GetTextBoundsWidth()     const { return m_TextBoundsWidth; }
        float       GetTextBoundsHeight()    const { return m_TextBoundsHeight; }
        float       GetTextMaxBearingUpper() const { return m_TextMaxBearingUpper; }

    private:
        Font*       m_Font;
        std::string m_Text;
        float       m_Width;
        float       m_Height;
        float       m_Scale;

        float       m_TextBoundsWidth;
        float       m_TextBoundsHeight;
        float       m_TextMaxBearingUpper;
    };
}