#include "RabBitCommon.h"
#include "GBuffer.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"

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
		auto meshes = scene->GetComponentsWithTypeOf<Entity::Mesh>();

		VertexBuffer** buffers = (VertexBuffer**) ALLOC_HEAP(sizeof(VertexBuffer*) * meshes.size());

		for (int i = 0; i < meshes.size(); ++i)
		{
			buffers[i] = ((const Entity::Mesh*)meshes[i])->GetVertexBuffer();
		}

		GBufferEntry* entry = new GBufferEntry();
		entry->buffers = buffers;
		entry->totalBuffers = meshes.size();

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

		for (int i = 0; i < entry->totalBuffers; ++i)
		{
			VertexBuffer* buffer = entry->buffers[i];

			render_interface->SetVertexBuffer(buffer);

			render_interface->Draw();
		}
	}
}