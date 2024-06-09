#pragma once
#include "graphics/RenderPass.h"

namespace RB::Graphics
{
	class GBufferPass : public RenderPass
	{
	public:
		RenderPassConfig GetConfiguration() override;

		RenderPassEntry* SubmitEntry(const Entity::Scene* const scene) override;

		void Render(RenderInterface* render_interface,
			ViewContext* view_context,
			RenderPassEntry* entry_context,
			RenderResource** output_textures,
			RenderResource** working_textures,
			RenderResource** dependency_textures) override;
	};
}