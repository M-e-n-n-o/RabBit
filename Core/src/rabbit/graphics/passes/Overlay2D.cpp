#include "RabBitCommon.h"
#include "Overlay2D.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"
#include "entity/components/Transform.h"
#include "entity/components/UI.h"

#include "graphics/shaders/shared/Common.h"
#include "graphics/codeGen/ShaderDefines.h"

#include <variant>

using namespace RB::Entity;

namespace RB::Graphics
{
    enum class OverlayEntryType
    {
        Rectangle,
        Text
    };

    struct Overlay2DEntry : public RenderPassEntry
    {
        struct Rectangle
        {
            float triangleData[8];
            Math::Float4 color;
        };

        struct Text
        {
            struct CharacterVB
            {
                float x;
                float y;
                float u;
                float v;
            };

            Texture2D* fontTex;
            Viewport scissor;
            List<CharacterVB> characters;
        };

        struct Element
        {
            OverlayEntryType type;
            uint32_t renderOrder;
            std::variant<Rectangle, Text> data;
        };

        List<Element> elements;

        ~Overlay2DEntry()
        {
        }
    };

    RenderPassConfig Overlay2DPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const Overlay2DSettings& s = (const Overlay2DSettings&)setting;

        return RenderPassConfig(
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"Color", 0}
                },

                // Working textures
                {},

                // Output textures
                {
                    RenderTextureDesc{"ColorOverlay",  RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRenderTarget},
                },

            // Async compute compatible
            false
            });
    }

    RenderPassEntry* Overlay2DPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene)
    {
        List<Overlay2DEntry::Element> elements;

        auto rectangles = scene->GetComponentsWithTypeOf<Rect2D>();
        for (int i = 0; i < rectangles.size(); ++i)
        {
            const Rect2D* rect = (const Rect2D*)rectangles[i];
            const Transform* transform = rectangles[i]->GetGameObject()->GetComponent<Transform>();

            float x0 = transform->position.x - rect->width * 0.5f;
            float x1 = transform->position.x + rect->width * 0.5f;
            float y0 = transform->position.y - rect->height * 0.5f;
            float y1 = transform->position.y + rect->height * 0.5f;

            Overlay2DEntry::Rectangle out_rect;
            out_rect.color  = rect->color;
            // Top left
            out_rect.triangleData[0] = x0;
            out_rect.triangleData[1] = y0;
            // Top right
            out_rect.triangleData[2] = x1;
            out_rect.triangleData[3] = y0;
            // Bottom left
            out_rect.triangleData[4] = x0;
            out_rect.triangleData[5] = y1;
            // Bottom right
            out_rect.triangleData[6] = x1;
            out_rect.triangleData[7] = y1;

            Overlay2DEntry::Element element = {};
            element.type        = OverlayEntryType::Rectangle;
            element.renderOrder = rect->GetRenderOrder();
            element.data        = out_rect;

            elements.push_back(element);
        }

        auto texts = scene->GetComponentsWithTypeOf<Text2D>();
        for (int tex_idx = 0; tex_idx < texts.size(); ++tex_idx)
        {
            const Text2D* text = (const Text2D*)texts[tex_idx];
            const Transform* transform = texts[tex_idx]->GetGameObject()->GetComponent<Transform>();
            const float scale = text->GetScale();

            // Calculate the baseline start position
            float start_x = transform->position.x;
            float start_y = transform->position.y + (text->GetTextMaxBearingUpper() * scale);

            Overlay2DEntry::Text out_text;
            out_text.fontTex    = text->GetFont()->GetFontTexture();
            out_text.scissor    = { (uint32_t)transform->position.x, (uint32_t)transform->position.y, uint32_t(text->GetWidth() * scale), uint32_t(text->GetHeight() * scale) };

            auto char_map = text->GetFont()->GetCharacterMap();
            for (char c : text->GetText())
            {
                const auto& ch = char_map->at(c);
        
                float xpos = start_x + ch.bearing.x * scale;
                float ypos = start_y + (ch.size.y - ch.bearing.y) * scale;
        
                float w = ch.size.x * scale;
                float h = ch.size.y * scale;

                // Top left
                Overlay2DEntry::Text::CharacterVB vertex0;
                vertex0.x = xpos;
                vertex0.y = ypos - h;
                vertex0.u = ch.imageUV.x;
                vertex0.v = ch.imageUV.w;
                // Top right
                Overlay2DEntry::Text::CharacterVB vertex1;
                vertex1.x = xpos + w;
                vertex1.y = ypos - h;
                vertex1.u = ch.imageUV.z;
                vertex1.v = ch.imageUV.w;
                // Bottom left
                Overlay2DEntry::Text::CharacterVB vertex2;
                vertex2.x = xpos;
                vertex2.y = ypos;
                vertex2.u = ch.imageUV.x;
                vertex2.v = ch.imageUV.y;
                // Bottom right
                Overlay2DEntry::Text::CharacterVB vertex3;
                vertex3.x = xpos + w;
                vertex3.y = ypos;
                vertex3.u = ch.imageUV.z;
                vertex3.v = ch.imageUV.y;

                start_x += ch.advance * scale;
                out_text.characters.push_back(vertex0);
                out_text.characters.push_back(vertex1);
                out_text.characters.push_back(vertex2);
                out_text.characters.push_back(vertex3);
            }

            Overlay2DEntry::Element element = {};
            element.type        = OverlayEntryType::Text;
            element.renderOrder = text->GetRenderOrder();
            element.data        = out_text;  

            elements.push_back(element);
        }
        
        if (elements.empty())
        {
            return nullptr;
        }

        // Sort the elements based on rendering order
        std::sort(elements.begin(), elements.end(), [](const Overlay2DEntry::Element& a, const Overlay2DEntry::Element& b) -> bool
        {
            return a.renderOrder < b.renderOrder;
        });

        Overlay2DEntry* entry = new Overlay2DEntry();
        entry->elements = elements;

        return entry;
    }

    void Overlay2DPass::Render(RenderPassInput& in)
    {
        in.ri->SetBlendMode(BlendMode::SrcAlphaLerp);
        in.ri->SetCullMode(CullMode::Back);
        in.ri->SetDepthMode(DepthMode::PassAll, false, false);

        in.ri->PushRenderTarget(in.outputTextures[0]);

        Frustum frustum;
        frustum.SetOrthographicProjection(0.0f, 1.0f, 0.0f, in.viewContext->viewport.width, 0.0f, in.viewContext->viewport.height, false);
        in.ri->SetConstantShaderData(kInstanceCB, &frustum.GetViewToClipMatrix(), sizeof(Math::Float4x4));

        auto RenderRectangle = [&](const Overlay2DEntry::Rectangle& rect)
        {
            in.ri->SetVertexShader(VS_Simple2D);
            in.ri->SetPixelShader(PS_Simple2D);

            Viewport vp;
            vp.left   = 0;
            vp.top    = 0;
            vp.width  = LONG_MAX;
            vp.height = LONG_MAX;
            in.ri->SetScissor(vp);

            float vertex_data[] =
            {   // Pos                                        UV                  Color
                rect.triangleData[0], rect.triangleData[1],   /*0.0f, 0.0f,*/     rect.color.x, rect.color.y, rect.color.z, rect.color.w,
                rect.triangleData[2], rect.triangleData[3],   /*1.0f, 0.0f,*/     rect.color.x, rect.color.y, rect.color.z, rect.color.w,
                rect.triangleData[4], rect.triangleData[5],   /*0.0f, 1.0f,*/     rect.color.x, rect.color.y, rect.color.z, rect.color.w,
                rect.triangleData[6], rect.triangleData[7],   /*1.0f, 1.0f,*/     rect.color.x, rect.color.y, rect.color.z, rect.color.w,
            };

            // Transient buffer
            auto vb = VertexBuffer::Create("Overlay Element", TopologyType::TriangleStrip, vertex_data, 6 * sizeof(float), sizeof(vertex_data), true);

            in.ri->SetVertexBuffer(vb);

            in.ri->Draw();

            SAFE_DELETE(vb);
        };

        auto RenderText = [&](const Overlay2DEntry::Text& text)
        {
            in.ri->SetVertexShader(VS_Font2D);
            in.ri->SetPixelShader(PS_Font2D);
            in.ri->SetScissor(text.scissor);

            for (int i = 0; i < text.characters.size(); i += 4)
            {
                const auto& vertex0 = text.characters[i + 0];
                const auto& vertex1 = text.characters[i + 1];
                const auto& vertex2 = text.characters[i + 2];
                const auto& vertex3 = text.characters[i + 3];

                float vertex_data[] =
                {
                    vertex0.x, vertex0.y, vertex0.u, vertex0.v,
                    vertex1.x, vertex1.y, vertex1.u, vertex1.v,
                    vertex2.x, vertex2.y, vertex2.u, vertex2.v,
                    vertex3.x, vertex3.y, vertex3.u, vertex3.v,
                };

                // Transient buffer
                auto vb = VertexBuffer::Create("Text Element", TopologyType::TriangleStrip, vertex_data, 4 * sizeof(float), sizeof(vertex_data), true);

                in.ri->SetVertexBuffer(vb);

                in.ri->SetShaderResourceInput(text.fontTex, 0);

                in.ri->Draw();

                SAFE_DELETE(vb);
            }
        };

        Overlay2DEntry* entry = (Overlay2DEntry*)in.entryContext;
        for (int i = 0; i < entry->elements.size(); i++)
        {
            switch (entry->elements[i].type)
            {
            case OverlayEntryType::Rectangle:
                RenderRectangle(std::get<Overlay2DEntry::Rectangle>(entry->elements[i].data));
                break;

            case OverlayEntryType::Text:
                RenderText(std::get<Overlay2DEntry::Text>(entry->elements[i].data));
                break;

            default:
                RB_LOG_WARN(LOGTAG_GRAPHICS, "UI type rendering not yet supported");
                break;
            }
        }
    }
}