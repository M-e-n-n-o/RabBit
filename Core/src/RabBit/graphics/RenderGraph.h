#pragma once

#include "RenderPass.h"

namespace RB::Graphics
{
    struct RenderPassConnection
    {
        RenderPassType from;
        RenderPassType to;

        uint32_t fromResources[8];
        uint32_t toResources[8];
    };

    // TODO Later in the RabBit editor you should be able to build this graph using a sort of shader graph (see EA SEED's Gigi)

    class RenderGraphBuilder
    {
    public:
        RenderGraphBuilder();

        RenderGraphBuilder& AddLink()
        {
            return *this;
        }

        RenderGraph Build();

    private:
        UnorderedMap<RenderPassType, RenderPass>  m_Passes;
        List<RenderPassConnection>                m_Connections;
    };

    class RenderGraph
    {
    public:
        virtual ~RenderGraph() = default;

        //void RunGraph(ViewContext*);

    private:
        
    };
}