#include "RabBitCommon.h"
#include "Overlay2D.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/View.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"
#include "entity/components/Transform.h"
#include "entity/components/Rect2D.h"

#include "graphics/shaders/shared/Common.h"
#include "graphics/codeGen/ShaderDefines.h"

using namespace RB::Entity;

namespace RB::Graphics
{
    struct Overlay2DEntry : public RenderPassEntry
    {
        struct Element
        {
            float triangleData[8];
        };

        Element* elements;
        uint32_t elementCount;
        
        struct CharacterVertex
        {
            float x;
            float y;
            float u;
            float v;
        };

        Texture2D* fontTex;
        CharacterVertex* characters;
        uint32_t characterCount;

        ~Overlay2DEntry()
        {
            SAFE_FREE(elements);
            SAFE_FREE(characters);
        }
    };

    RenderPassConfig Overlay2DPass::GetConfiguration(const RenderPassSettings& setting)
    {
        const Overlay2DSettings& s = (const Overlay2DSettings&)setting;

        return RenderPassConfig(
            {
                // Dependencies
                {
                    RenderTextureInputDesc{"Color", false, 0}
                },
                1,

            // Working textures
            {},
            0,

            // Output textures
            {
                RenderTextureDesc{"ColorOverlay",  RenderResourceFormat::R32G32B32A32_FLOAT, kRTSize_Full, kRTSize_Full, kRTFlag_AllowRenderTarget},
            },
            1,

            // Async compute compatible
            false
            });
    }

    RenderPassEntry* Overlay2DPass::SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene)
    {
        auto rectangles = scene->GetComponentsWithTypeOf<Rect2D>();
        Overlay2DEntry::Element* elements = ALLOC_HEAPC(Overlay2DEntry::Element, rectangles.size());
        uint32_t total_elements = 0;

        for (int i = 0; i < rectangles.size(); ++i)
        {
            const Rect2D* rect = (const Rect2D*)rectangles[i];
            const Transform* transform = rectangles[i]->GetGameObject()->GetComponent<Transform>();

            float x0 = transform->position.x - rect->width * 0.5f;
            float x1 = transform->position.x + rect->width * 0.5f;
            float y0 = transform->position.y - rect->height * 0.5f;
            float y1 = transform->position.y + rect->height * 0.5f;

            Overlay2DEntry::Element element = {};
            // Top left
            element.triangleData[0] = x0;
            element.triangleData[1] = y0;
            // Top right
            element.triangleData[2] = x1;
            element.triangleData[3] = y0;
            // Bottom left
            element.triangleData[4] = x0;
            element.triangleData[5] = y1;
            // Bottom right
            element.triangleData[6] = x1;
            element.triangleData[7] = y1;

            elements[total_elements] = element;
            total_elements++;
        }

        auto texts = scene->GetComponentsWithTypeOf<Text2D>();
        uint32_t expected_characters = 0;
        for (int tex_idx = 0; tex_idx < texts.size(); ++tex_idx)
        {
            const Text2D* text = (const Text2D*)texts[tex_idx];
            expected_characters += text->font->GetCharacterMap()->size();
        }

        Overlay2DEntry::CharacterVertex* character_vertices = ALLOC_HEAPC(Overlay2DEntry::CharacterVertex, expected_characters * 4);
        uint32_t total_vertices = 0;
        
        for (int tex_idx = 0; tex_idx < texts.size(); ++tex_idx)
        {
            const Text2D* text = (const Text2D*)texts[tex_idx];
            const Transform* transform = texts[tex_idx]->GetGameObject()->GetComponent<Transform>();
            const float scale = 1.0f;

            auto char_map = text->font->GetCharacterMap();
        
            float start_x = transform->position.x;
            float start_y = transform->position.y;
            for (char c : text->text)
            {
                const auto& ch = char_map->at(c);
        
                float xpos = start_x + ch.bearing.x * scale;
                float ypos = start_y + (ch.size.y - ch.bearing.y) * scale;
        
                float w = ch.size.x * scale;
                float h = ch.size.y * scale;

                // Top left
                character_vertices[total_vertices + 0].x = xpos;
                character_vertices[total_vertices + 0].y = ypos - h;
                character_vertices[total_vertices + 0].u = ch.imageUV.x;
                character_vertices[total_vertices + 0].v = ch.imageUV.w;
                // Top right
                character_vertices[total_vertices + 1].x = xpos + w;
                character_vertices[total_vertices + 1].y = ypos - h;
                character_vertices[total_vertices + 1].u = ch.imageUV.z;
                character_vertices[total_vertices + 1].v = ch.imageUV.w;
                // Bottom left
                character_vertices[total_vertices + 2].x = xpos;
                character_vertices[total_vertices + 2].y = ypos;
                character_vertices[total_vertices + 2].u = ch.imageUV.x;
                character_vertices[total_vertices + 2].v = ch.imageUV.y;
                // Bottom right
                character_vertices[total_vertices + 3].x = xpos + w;
                character_vertices[total_vertices + 3].y = ypos;
                character_vertices[total_vertices + 3].u = ch.imageUV.z;
                character_vertices[total_vertices + 3].v = ch.imageUV.y;

                total_vertices += 4;
                start_x += ch.advance * scale;
            }
        }
        
        if (total_elements == 0 && total_vertices == 0)
        {
            SAFE_FREE(elements);
            SAFE_FREE(character_vertices);
            return nullptr;
        }

        Overlay2DEntry* entry = new Overlay2DEntry();
        entry->elements         = elements;
        entry->elementCount     = total_elements;
        entry->fontTex          = ((const Text2D*)texts[0])->font->GetFontTexture();
        entry->characters       = character_vertices;
        entry->characterCount   = total_vertices;

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

        Overlay2DEntry* entry = (Overlay2DEntry*)in.entryContext;


        // Render 2D rectangles
        in.ri->SetVertexShader(VS_Simple2D);
        in.ri->SetPixelShader(PS_Simple2D);
        for (int i = 0; i < entry->elementCount; ++i)
        {
            const Overlay2DEntry::Element& e = entry->elements[i];

            float vertex_data[] =
            {   // Pos                                  UV                  Color
                e.triangleData[0], e.triangleData[1],   /*0.0f, 0.0f,*/     1, 0, 0,
                e.triangleData[2], e.triangleData[3],   /*1.0f, 0.0f,*/     0, 1, 0,
                e.triangleData[4], e.triangleData[5],   /*0.0f, 1.0f,*/     0, 0, 1,
                e.triangleData[6], e.triangleData[7],   /*1.0f, 1.0f,*/     0, 1, 1,
            };

            // Transient buffer
            auto vb = VertexBuffer::Create("Overlay Element", TopologyType::TriangleStrip, vertex_data, 5 * sizeof(float), sizeof(vertex_data), true);

            in.ri->SetVertexBuffer(vb);

            in.ri->SetConstantShaderData(kInstanceCB, &frustum.GetViewToClipMatrix(), sizeof(Math::Float4x4));

            in.ri->Draw();

            SAFE_DELETE(vb);
        }

        // Render text
        in.ri->SetVertexShader(VS_Font2D);
        in.ri->SetPixelShader(PS_Font2D);
        for (int i = 0; i < entry->characterCount; i += 4)
        {
            const Overlay2DEntry::CharacterVertex& vertex0 = entry->characters[i + 0];
            const Overlay2DEntry::CharacterVertex& vertex1 = entry->characters[i + 1];
            const Overlay2DEntry::CharacterVertex& vertex2 = entry->characters[i + 2];
            const Overlay2DEntry::CharacterVertex& vertex3 = entry->characters[i + 3];

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

            in.ri->SetConstantShaderData(kInstanceCB, &frustum.GetViewToClipMatrix(), sizeof(Math::Float4x4));

            in.ri->SetShaderResourceInput(entry->fontTex, 0);

            in.ri->Draw();

            SAFE_DELETE(vb);
        }

    }
}