#pragma once

#include "RabBitCommon.h"

namespace RB::Graphics
{
    class RenderInterface;
    class RenderResource;
    class GpuGuard;

    struct Streamable
    {
        RenderResource* resource;
        void*           uploadData;
        uint64_t		uploadSize;
    };

    class ResourceStreamer
    {
    public:
        ResourceStreamer();
        ~ResourceStreamer();

        void ScheduleForStream(const Streamable& streamable);

        Shared<GpuGuard> Stream(RenderInterface* render_interface);

    private:
        void UpdateStreamedEntries();

        struct StreamedEntry
        {
            Shared<GpuGuard> guard;
            List<Streamable> streamables;
        };

        Queue<Streamable>	 m_Streamables;
        List<StreamedEntry>	 m_StreamedEntries;
    };
}