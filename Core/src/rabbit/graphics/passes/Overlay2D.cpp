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

            Shared<Texture2D> fontTex;
            Viewport scissor;
            List<CharacterVB> vertices;
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
        uint32_t window_width  = view_context->viewport.width;
        uint32_t window_height = view_context->viewport.height;

        List<Overlay2DEntry::Element> elements;

        auto canvases = scene->GetComponentsWithTypeOf<UICanvas>();
        for (const ObjectComponent* obj : canvases)
        {
            const UICanvas* canvas = (const UICanvas*)obj;
            if (canvas->GetTargetCamera() != view_context->camera || !canvas->IsEnabled())
            {
                continue;
            }

            List<UIRenderComponent*> render_comps;
            canvas->GetGameObject()->GetComponentsInChildren<UIRenderComponent>(render_comps);

            for (int i = 0; i < render_comps.size(); ++i)
            {
                if (!render_comps[i]->IsEnabled())
                {
                    continue;
                }

                const UIBox* box = render_comps[i]->GetGameObject()->GetComponent<UIBox>();
                if (box == nullptr)
                {
                    RB_LOG_WARN(LOGTAG_ENTITY, "Every UI object should have a UIBox");
                    continue;
                }

                const auto [pos_x, pos_y] = box->GetWorldStartPos();
                const auto [bounds_x, bounds_y] = box->GetWorldSize();

                if (auto rect = dynamic_cast<const Rect2D*>(render_comps[i]); rect != nullptr)
                {

                    float x0 = pos_x;
                    float x1 = pos_x + bounds_x;
                    float y0 = pos_y;
                    float y1 = pos_y + bounds_y;

                    Overlay2DEntry::Rectangle out_rect;
                    out_rect.color  = rect->GetColor();
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
                else if (auto text = dynamic_cast<const Text2D*>(render_comps[i]); text != nullptr)
                {
                    const float scale = text->GetScale();

                    // Calculate the baseline start position
                    float start_x = pos_x;
                    float start_y = pos_y + (text->GetTextMaxBearingUpper() * scale);

                    Overlay2DEntry::Text out_text;
                    out_text.fontTex    = text->GetFont()->GetFontTexture();
                    out_text.scissor    = { (uint32_t)pos_x, (uint32_t)pos_y, uint32_t(bounds_x * scale), uint32_t(bounds_y * scale) };

                    auto char_map = text->GetFont()->GetCharacterMap();
                    for (char c : text->GetText())
                    {
                        const auto& ch = char_map->at(c);
        
                        float xpos = start_x + ch.bearing.x * scale;
                        float ypos = start_y + (ch.size.y - ch.bearing.y) * scale;
        
                        float w = ch.size.x * scale;
                        float h = ch.size.y * scale;

                        // Top left
                        Overlay2DEntry::Text::CharacterVB t0v0;
                        t0v0.x = xpos;
                        t0v0.y = ypos - h;
                        t0v0.u = ch.imageUV.x;
                        t0v0.v = ch.imageUV.w;
                        // Top right
                        Overlay2DEntry::Text::CharacterVB t0v1;
                        t0v1.x = xpos + w;
                        t0v1.y = ypos - h;
                        t0v1.u = ch.imageUV.z;
                        t0v1.v = ch.imageUV.w;
                        // Bottom left
                        Overlay2DEntry::Text::CharacterVB t0v2;
                        t0v2.x = xpos;
                        t0v2.y = ypos;
                        t0v2.u = ch.imageUV.x;
                        t0v2.v = ch.imageUV.y;

                        // Top right
                        Overlay2DEntry::Text::CharacterVB t1v0;
                        t1v0.x = xpos + w;
                        t1v0.y = ypos - h;
                        t1v0.u = ch.imageUV.z;
                        t1v0.v = ch.imageUV.w;
                        // Bottom right
                        Overlay2DEntry::Text::CharacterVB t1v1;
                        t1v1.x = xpos + w;
                        t1v1.y = ypos;
                        t1v1.u = ch.imageUV.z;
                        t1v1.v = ch.imageUV.y;
                        // Bottom left
                        Overlay2DEntry::Text::CharacterVB t1v2;
                        t1v2.x = xpos;
                        t1v2.y = ypos;
                        t1v2.u = ch.imageUV.x;
                        t1v2.v = ch.imageUV.y;

                        start_x += ch.advance * scale;
                        out_text.vertices.push_back(t0v0);
                        out_text.vertices.push_back(t0v1);
                        out_text.vertices.push_back(t0v2);
                        out_text.vertices.push_back(t1v0);
                        out_text.vertices.push_back(t1v1);
                        out_text.vertices.push_back(t1v2);
                    }

                    Overlay2DEntry::Element element = {};
                    element.type        = OverlayEntryType::Text;
                    element.renderOrder = text->GetRenderOrder();
                    element.data        = out_text;  

                    elements.push_back(element);
                }
            }
        }


        auto components = scene->GetComponentsWithTypeOf<UIRenderComponent>();
        
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

            in.ri->SetVertexBuffer(vb.get());

            in.ri->Draw();
        };

        auto RenderText = [&](const Overlay2DEntry::Text& text)
        {
            in.ri->SetVertexShader(VS_Font2D);
            in.ri->SetPixelShader(PS_Font2D);
            in.ri->SetScissor(text.scissor);

            uint32_t vertex_size = sizeof(float) * 4;
            uint32_t data_size = vertex_size * text.vertices.size();
            float* vertex_data = (float*)ALLOC_STACK(data_size);

            for (int v_idx = 0; v_idx < text.vertices.size(); v_idx++)
            {
                const auto& vertex = text.vertices[v_idx];
                vertex_data[v_idx * 4 + 0] = vertex.x;
                vertex_data[v_idx * 4 + 1] = vertex.y;
                vertex_data[v_idx * 4 + 2] = vertex.u;
                vertex_data[v_idx * 4 + 3] = vertex.v;
            }

            // Transient buffer
            auto vb = VertexBuffer::Create("Text Element", TopologyType::TriangleList, vertex_data, vertex_size, data_size, true);
            in.ri->SetVertexBuffer(vb.get());

            in.ri->SetShaderResourceInput(text.fontTex.get(), 0);

            in.ri->Draw();
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