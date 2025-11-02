#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "app/AssetManager.h"
#include "graphics/RenderResource.h"

namespace RB::Entity
{
    class Camera;

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
    //                             UI Base Components
    // ---------------------------------------------------------------------------

    enum class UIUnit
    {
        PX = 0, // Pixels
        PCT,    // Percentage of parent (0-100)
        IPCT    // Independent percentage of parent (percentage of smallest side) (0-100)
    };

    // UI always starts with a UICanvas at the head
    class UICanvas : public ObjectComponent
    {
    public:
        UICanvas(Camera* target_camera)
            :m_TargetCamera(target_camera)
        {}

        virtual ~UICanvas() = default;

        Camera* GetTargetCamera() const { return m_TargetCamera; }

    private:
        Camera* m_TargetCamera;
    };

    enum class UIConstraintType
    {
        Left,
        Right,
        Up,
        Down
    };

    // Every UI gameobject should have a UIBox (this is sort of the Transform for 3D objects)
    class UIBox : public ObjectComponent
    {
    public:
        UIBox();
        virtual ~UIBox() = default;
    
        void SetStartPos(UIUnit unit, float x, float y);
        void SetSize(UIUnit unit, float width, float height);
        
        //void SetMargin(UIUnit unit, float x, float y);
        void SetPadding(UIUnit unit, float padding); // Only applies to constraints
        void AddConstraint(UIConstraintType type);

        Pair<float, float> GetStartPos(UIUnit unit) const;
        Pair<float, float> GetSize(UIUnit unit) const;

        // Returns the final translation values in pixels
        Pair<float, float> GetWorldStartPos() const;
        Pair<float, float> GetWorldSize() const;

        void Update() override;

    private:
        void UpdateMagnets();
        void GetParentBounds(float& x, float& y, float& width, float& height) const;
        Pair<float, float> ConvertUnits(UIUnit current, UIUnit target, float x, float y) const;

        UIUnit m_PosUnit;
        float  m_PosX;
        float  m_PosY;
        UIUnit m_SizeUnit;
        float  m_Width;
        float  m_Height;
        UIUnit m_PaddingUnit;
        float  m_Padding;
        bool   m_LeftMagnet;
        bool   m_RightMagnet;
        bool   m_TopMagnet;
        bool   m_BottomMagnet;
        float  m_LastParentWidth;
        float  m_LastParentHeight;
    };

    // ---------------------------------------------------------------------------
    //                               UI Components
    // ---------------------------------------------------------------------------

    class UIComponent : public ObjectComponent
    {
    public:
        virtual ~UIComponent() = default;
    };

    // A ListView only organizes direct children!
    class ListView : public UIComponent
    {
    public:
        ListView(bool vertical, float element_padding);

        void OnChildAttached(GameObject* obj) override;
        void OnChildDettached(GameObject* obj) override;

    private:
        void UpdateItemPositions();

        bool     m_Vertical;
        UIBox*   m_Box;
        float    m_ElementPadding;
    };
    REGISTER_COMP_BASES(ListView, UIComponent);

    class UIRenderComponent : public UIComponent
    {
    public:
        UIRenderComponent(uint32_t render_order = 0)
            : m_RenderOrder(render_order)
        {
        }
        virtual ~UIRenderComponent() = default;

        int GetRenderOrder() const { return m_RenderOrder; }

    private:
        uint32_t m_RenderOrder;
    };
    REGISTER_COMP_BASES(UIRenderComponent, UIComponent);

    class Rect2D : public UIRenderComponent
    {
    public:

        Rect2D(Math::Float4 color, float render_order = 0)
            : UIRenderComponent(render_order)
            , m_Color(color)
        {
        }

        Math::Float4 GetColor() const { return m_Color; }

    private:
        Math::Float4 m_Color;
    };
    REGISTER_COMP_BASES(Rect2D, UIRenderComponent, UIComponent);

    class Text2D : public UIRenderComponent
    {
    public:
        Text2D(Font* font, const std::string& text, float scale = 1.0f, float render_order = 0);

        void UpdateText(const std::string& text);

        const Font* GetFont()   const { return m_Font; }
        std::string GetText()   const { return m_Text; }
        float       GetScale()  const { return m_Scale; }
        
        float       GetTextBoundsWidth()     const { return m_TextBoundsWidth; }
        float       GetTextBoundsHeight()    const { return m_TextBoundsHeight; }
        float       GetTextMaxBearingUpper() const { return m_TextMaxBearingUpper; }

    private:
        Font*        m_Font;
        std::string  m_Text;
        float        m_Scale;
        float        m_TextBoundsWidth;
        float        m_TextBoundsHeight;
        float        m_TextMaxBearingUpper;
    };
    REGISTER_COMP_BASES(Text2D, UIRenderComponent, UIComponent);
}