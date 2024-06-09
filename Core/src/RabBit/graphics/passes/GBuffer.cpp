#include "RabBitCommon.h"
#include "GBuffer.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"
#include "graphics/ViewContext.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"
#include "entity/components/Transform.h"

#include "graphics/d3d12/shaders/shared/test2.h"

namespace RB::Graphics
{
	struct GBufferEntry : public RenderPassEntry
	{
		struct ModelEntry
		{
			VertexBuffer*  vb;
			IndexBuffer*   ib;
			Math::Float4x4 modelMatrix;
		};

		ModelEntry* entries;
		uint32_t totalEntries;

		~GBufferEntry()
		{
			SAFE_FREE(entries);
		}
	};

	RenderPassConfig GBufferPass::GetConfiguration()
	{
		return RenderPassConfigBuilder(RenderPassType::GBuffer, "GBuffer", false).Build();
	}

	RenderPassEntry* GBufferPass::SubmitEntry(const Entity::Scene* const scene)
	{
		auto mesh_renderers = scene->GetComponentsWithTypeOf<Entity::MeshRenderer>();

		GBufferEntry::ModelEntry* entries = (GBufferEntry::ModelEntry*) ALLOC_HEAP(sizeof(GBufferEntry::ModelEntry) * mesh_renderers.size());

		uint32_t total_entries = 0;

		for (int i = 0; i < mesh_renderers.size(); ++i)
		{
			const Entity::MeshRenderer* mesh_renderer = (const Entity::MeshRenderer*)mesh_renderers[i];
			const Entity::Mesh* mesh = mesh_renderer->GetMesh();

			if (!mesh->IsLatestDataUploaded())
			{
				continue;
			}

			const Entity::Transform* transform = mesh_renderer->GetGameObject()->GetComponent<Entity::Transform>();

			if (transform == nullptr)
			{
				continue;
			}

			GBufferEntry::ModelEntry entry = {};
			entry.vb			= mesh->GetVertexBuffer();
			entry.ib			= mesh->GetIndexBuffer();
			entry.modelMatrix	= transform->GetLocalToWorldMatrix();

			entries[total_entries] = entry;

			total_entries++;
		}

		if (total_entries == 0)
		{
			delete entries;
			return nullptr;
		}

		GBufferEntry* entry = new GBufferEntry();
		entry->entries		= entries;
		entry->totalEntries = total_entries;

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
		transform.worldToViewMat = view_context->viewFrustum.GetWorldToViewMatrix();
		//transform.worldToViewMat.Transpose();
		transform.viewToClipMat = view_context->viewFrustum.GetViewToClipMatrix();
		//transform.viewToClipMat.Transpose();

		ColorCB color;
		color.color = { 0.0f, 1.0f, 0.0f };

		for (int i = 0; i < entry->totalEntries; ++i)
		{
			GBufferEntry::ModelEntry& model_entry = entry->entries[i];

			render_interface->SetVertexBuffer(model_entry.vb);

			if (model_entry.ib)
			{
				render_interface->SetIndexBuffer(model_entry.ib);
			}

			transform.localToWorldMat = model_entry.modelMatrix;
			//transform.localToWorldMat.Transpose();

			Math::Float4x4 mvp = transform.localToWorldMat * transform.worldToViewMat;
			mvp = mvp * transform.viewToClipMat;
			transform.localToWorldMat = mvp;

			render_interface->SetConstantShaderData(0, &transform, sizeof(transform));

			render_interface->Draw();
		}
	}
}