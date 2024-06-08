#include "RabBitCommon.h"
#include "GBuffer.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"

#include "graphics/d3d12/shaders/shared/test2.h"

namespace RB::Graphics
{
	struct GBufferEntry : public RenderPassEntry
	{
		VertexBuffer**  buffers;
		uint32_t		totalBuffers;

		~GBufferEntry()
		{
			delete buffers;
		}
	};

	RenderPassConfig GBufferPass::GetConfiguration()
	{
		return RenderPassConfigBuilder(RenderPassType::GBuffer, "GBuffer", false).Build();
	}

	RenderPassEntry* GBufferPass::SubmitEntry(ViewContext* view_context, const Entity::Scene* const scene)
	{
		auto mesh_renderers = scene->GetComponentsWithTypeOf<Entity::MeshRenderer>();

		// Allocate for the worst case scenario amount of vertex buffers
		VertexBuffer** buffers = (VertexBuffer**) ALLOC_HEAP(sizeof(VertexBuffer*) * mesh_renderers.size());

		uint32_t total_meshes = 0;

		for (int i = 0; i < mesh_renderers.size(); ++i)
		{
			const Entity::Mesh* mesh = ((const Entity::MeshRenderer*)mesh_renderers[i])->GetMesh();

			if (!mesh->IsLatestDataUploaded())
			{
				continue;
			}

			bool already_inserted = false;
			for (int j = 0; j < total_meshes; ++j)
			{
				if (buffers[j] == mesh->GetVertexBuffer())
				{
					already_inserted = true;
					break;
				}
			}

			if (already_inserted)
			{
				continue;
			}

			buffers[total_meshes] = mesh->GetVertexBuffer();
			total_meshes++;
		}

		if (total_meshes == 0)
		{
			delete buffers;
			return nullptr;
		}

		GBufferEntry* entry = new GBufferEntry();
		entry->buffers = buffers;
		entry->totalBuffers = total_meshes;

		return entry;
	}

	void GBufferPass::Render(RenderInterface* render_interface, ViewContext* view_context, RenderPassEntry* entry_context,
		RenderResource** output_textures, RenderResource** working_textures, RenderResource** dependency_textures)
	{
		render_interface->SetVertexShader(VS_VertexColor);
		render_interface->SetPixelShader(PS_VertexColor);

		render_interface->SetBlendMode(BlendMode::None);
		render_interface->SetCullMode(CullMode::None);
		render_interface->SetDepthMode(DepthMode::Disabled);

		RenderTargetBundle bundle = {};
		bundle.colorTargetsCount	= 1;
		bundle.colorTargets[0]		= (Texture2D*)output_textures[0];
		bundle.depthStencilTarget	= nullptr;

		render_interface->SetRenderTarget(&bundle);

		GBufferEntry* entry = (GBufferEntry*) entry_context;

		TransformCB transform;
		transform.offset = { 0.75f, 0 };

		ColorCB color;
		color.color = { 0.0f, 1.0f, 0.0f };

		for (int i = 0; i < entry->totalBuffers; ++i)
		{
			VertexBuffer* buffer = entry->buffers[i];

			render_interface->SetVertexBuffer(buffer);

			render_interface->SetConstantShaderData(0, &transform, sizeof(transform));
			render_interface->SetConstantShaderData(1, &color, sizeof(color));

			render_interface->Draw();
		}
	}
}