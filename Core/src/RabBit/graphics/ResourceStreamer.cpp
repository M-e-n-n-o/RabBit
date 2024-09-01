#include "RabBitCommon.h"
#include "ResourceStreamer.h"
#include "RenderResource.h"
#include "RenderInterface.h"

namespace RB::Graphics
{
    ResourceStreamer::ResourceStreamer()
    {
    }

    ResourceStreamer::~ResourceStreamer()
    {
        // Clear the streamable queue
        {
            Queue<Streamable> empty;
            std::swap(m_Streamables, empty);
        }

        // Wait until all the resources have been streamed completely
        for (auto itr = m_StreamedEntries.begin(); itr != m_StreamedEntries.end(); ++itr)
        {
            itr->guard->WaitUntilFinishedRendering();

            for (int i = 0; i < itr->streamables.size(); ++i)
            {
                // Free the upload data and set the resource as being finished with streaming
                SAFE_FREE(itr->streamables[i].uploadData);
                itr->streamables[i].resource->SetIsStreaming(false);
            }
        }

        m_StreamedEntries.clear();
    }

    void ResourceStreamer::ScheduleForStream(const Streamable& streamable)
    {
        // Set the resource as streaming
        streamable.resource->SetIsStreaming(true);

        Streamable copy;
        copy.resource   = streamable.resource;
        copy.uploadSize = streamable.uploadSize;
        copy.uploadData = ALLOC_HEAP(copy.uploadSize);

        // Memcpy over the data so the RenderResource does not need to keep it around
        memcpy(copy.uploadData, streamable.uploadData, copy.uploadSize);

        m_Streamables.push(copy);
    }

    Shared<GpuGuard> ResourceStreamer::Stream(RenderInterface* render_interface)
    {
        UpdateStreamedEntries();

        // TODO Maybe stream only a max number of streamables per frame, or maybe give it a time budget
        // (unstreamed resources can just use a fallback if they are not very important, or use a lower 
        // mip if its a texture as textures will be streamed per mip).

        StreamedEntry streamed_entry = {};

        streamed_entry.streamables.reserve(m_Streamables.size());

        while (!m_Streamables.empty())
        {
            Streamable& streamable = m_Streamables.front();

            render_interface->UploadDataToResource(streamable.resource, streamable.uploadData, streamable.uploadSize);

            streamed_entry.streamables.push_back(streamable);
            m_Streamables.pop();
        }

        if (streamed_entry.streamables.empty())
        {
            // Did no streaming
            return nullptr;
        }

        // Execute the streaming on the GPU
        streamed_entry.guard = render_interface->ExecuteOnGpu();

        m_StreamedEntries.push_back(streamed_entry);

        return streamed_entry.guard;
    }

    void ResourceStreamer::UpdateStreamedEntries()
    {
        for (auto itr = m_StreamedEntries.begin(); itr != m_StreamedEntries.end();)
        {
            if (itr->guard->IsFinishedRendering())
            {
                for (int i = 0; i < itr->streamables.size(); ++i)
                {
                    // Free the upload data and set the resource as being finished with streaming
                    SAFE_FREE(itr->streamables[i].uploadData);
                    itr->streamables[i].resource->SetIsStreaming(false);
                }

                itr = m_StreamedEntries.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
    }
}