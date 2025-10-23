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

    // A ListView only organizes direct children!
    class ListView : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("ListView");

        ListView(bool vertical, uint32_t padding);

        void OnChildAttached(GameObject* obj) override;
        void OnChildDettached(GameObject* obj) override;

    private:
        void UpdateItemPositions();

        bool     m_Vertical;
        uint32_t m_Padding;
    };

    class UIMagnet : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("UIMagnet");

        UIMagnet(bool left, bool right, bool top, bool bottom, uint32_t padding);

        void Update() override;

    private:
        void UpdateItemPositions();

        bool      m_Left;
        bool      m_Right;
        bool      m_Top;
        bool      m_Bottom;
        uint32_t  m_Padding;
        uint32_t  m_LastWindowWidth;
        uint32_t  m_LastWindowHeight;
    };

    // UIRenderComponent:
    // - Always requires a Transform component as well
    //     - x, y should tell top left starting position of the UI component
    // - Sizes and positions of the UI component are specified in screen pixels
    class UIRenderComponent : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("UIRenderComponent");

        UIRenderComponent(uint32_t render_order = 0)
            : m_RenderOrder(render_order)
        {
        }

        int GetRenderOrder() const { return m_RenderOrder; }

        virtual const Math::Float2& GetBounds() const = 0;

    private:
        uint32_t m_RenderOrder;
    };

    class Rect2D : public UIRenderComponent
    {
    public:

        Rect2D(float width, float height, Math::Float4 color, float render_order = 0)
            : UIRenderComponent(render_order)
            , m_Color(color)
        {
            m_Bounds = Math::Float2(width, height);
        }

        Math::Float4 GetColor() const { return m_Color; }
        const Math::Float2& GetBounds() const override { return m_Bounds; }

    private:
        Math::Float2 m_Bounds;
        Math::Float4 m_Color;
    };

    class Text2D : public UIRenderComponent
    {
    public:
        //DEFINE_COMP_TAG("Text2D");

        Text2D(Font* font, const std::string& text, float scale = 1.0f, float width = -1, float height = -1, float render_order = 0);

        void UpdateText(const std::string& text);

        const Font* GetFont()   const { return m_Font; }
        std::string GetText()   const { return m_Text; }
        float       GetScale()  const { return m_Scale; }
        
        float       GetTextBoundsWidth()     const { return m_TextBoundsWidth; }
        float       GetTextBoundsHeight()    const { return m_TextBoundsHeight; }
        float       GetTextMaxBearingUpper() const { return m_TextMaxBearingUpper; }

        const Math::Float2& GetBounds() const override { return m_Bounds; }

    private:
        Font*        m_Font;
        std::string  m_Text;
        Math::Float2 m_Bounds;
        float        m_Scale;
                     
        float        m_TextBoundsWidth;
        float        m_TextBoundsHeight;
        float        m_TextMaxBearingUpper;
    };
}