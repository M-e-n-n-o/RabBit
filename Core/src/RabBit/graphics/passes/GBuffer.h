#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
	class GBufferPass : public RenderPass
	{
	public:
		RenderPassConfig GetConfiguration() override;

		Shared<RenderPassEntry> SubmitEntry(ViewContext* view_context, const Entity::Scene* const scene) override;

		void Render(RenderInterface* render_interface,
			ViewContext* view_context,
			Shared<RenderPassEntry> entry_context,
			RenderResource** output_textures,
			RenderResource** working_textures,
			RenderResource** dependency_textures) override;
	};
}