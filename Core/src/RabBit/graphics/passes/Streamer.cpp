#include "RabBitCommon.h"
#include "Streamer.h"

#include "graphics/RenderResource.h"
#include "graphics/RenderInterface.h"

#include "entity/Scene.h"
#include "entity/components/Mesh.h"

namespace RB::Graphics
{
	struct StreamerEntry : public RenderPassEntry
	{
		VertexBuffer**	buffers;
		uint32_t		totalBuffers;

		~StreamerEntry()
		{
			delete buffers;
		}
	};

	RenderPassConfig StreamerPass::GetConfiguration()
	{
		return RenderPassConfigBuilder(RenderPassType::Streamer, "Streamer", false).Build();
	}

	/*
		!!!!! RESOURCE STREAMING THREAD SHOULD ONLY SCHEDULE THINGS ON THE COPY INTERFACE, NO GRAPHICS STUFF !!!!!! 

		(Slowly) Upload needed resources for next frame, like new vertex data or textures.
		Constantly check if the main thread is waiting until this task is done, if so, finish uploading only the most crucial resources 
		and then stop (so textures are not completely crucial, as we can use the lower mips as fallback, or even white textures, so upload textures mip by mip).
		Maybe an idea to give this task always a minimum amount of time to let it upload resources, because when maybe the main thread
		is not doing a lot, this task will not get a lot of time actually uploading stuff.

		Do not forget to add GPU waits on the copy interface for the previous graphics interface before starting the texture copies!
		Also do not forget to let the render thread sync with the resource streaming just before the streaming has been done!

		TODO Maybe an idea to batch all some uploads together in a bigger upload resource?
	*/

	RenderPassEntry* StreamerPass::SubmitEntry(ViewContext* view_context, const Entity::Scene* const scene)
	{
		List<const Entity::ObjectComponent*> mesh_renderers = scene->GetComponentsWithTypeOf<Entity::MeshRenderer>();

		// Allocate for the worst case scenario amount of vertex buffers
		VertexBuffer** buffers = (VertexBuffer**)ALLOC_HEAP(sizeof(VertexBuffer*) * mesh_renderers.size());

		uint32_t uploaded_count = 0;

		for (const Entity::ObjectComponent* obj : mesh_renderers)
		{
			Entity::Mesh* mesh = ((const Entity::MeshRenderer*)obj)->GetMesh();

			if (mesh->IsLatestDataUploaded())
			{
				continue;
			}

			bool already_inserted = false;
			for (int i = 0; i < uploaded_count; ++i)
			{
				if (buffers[i] == mesh->GetVertexBuffer())
				{
					already_inserted = true;
					break;
				}
			}

			if (already_inserted)
			{
				continue;
			}

			buffers[uploaded_count] = mesh->GetVertexBuffer();
			uploaded_count++;

			// TODO This is ofcourse not true, they are uploaded once the fence has been reached, but how to do this in a proper and clean way?
			mesh->SetLatestDataUploaded(true);
		}

		StreamerEntry* entry = new StreamerEntry();
		entry->buffers		= buffers;
		entry->totalBuffers = uploaded_count;

		return entry;
	}

	void StreamerPass::Render(RenderInterface* render_interface, ViewContext* view_context, RenderPassEntry* entry_context,
		RenderResource** output_textures, RenderResource** working_textures, RenderResource** dependency_textures)
	{
		StreamerEntry* entry = (StreamerEntry*) entry_context;

		for (int i = 0; i < entry->totalBuffers; ++i)
		{
			render_interface->UploadDataToResource(entry->buffers[i], entry->buffers[i]->GetData(), entry->buffers[i]->GetSize());
		}
	}
}