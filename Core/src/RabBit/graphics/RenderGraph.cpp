#include "RabBitCommon.h"
#include "RenderGraph.h"

namespace RB::Graphics
{
    // ---------------------------------------------------------------------------
    //                               RenderGraph
    // ---------------------------------------------------------------------------

    // ---------------------------------------------------------------------------
    //                            RenderGraphBuilder
    // ---------------------------------------------------------------------------

    RenderGraphBuilder::RenderGraphBuilder()
        : m_FinalPassType(RenderPassType::None)
    {
        
    }

    RenderGraph RenderGraphBuilder::Build()
    {
        AddLink(RenderPassType::None, RenderPassType::None, 
                        0u,   /*  ->  */   0u,  // Output number 0 of left pass is input number 0 of the right pass
                        1u,   /*  ->  */   3u); // Output number 1 of left pass is input number 3 of the right pass

        /*
            - We first need to figure out the flow of the passes, making sure that passes do not run twice!
            - Then the lifetime of each RenderResource
            - We then need to figure out the flow of these resources between the passes
        */

        if (m_FinalPassType == RenderPassType::None)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "The final pass of the RenderGraph is not properly set yet");
            return {};
        }

        auto itr = m_Passes.find(m_FinalPassType);

        if (itr == m_Passes.end())
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "The final pass of the RenderGraph is not actually added as a pass");
            return {};
        }

        RenderPass*      final_pass   = itr->second;
        RenderPassConfig final_config = final_pass->GetConfiguration(m_PassSettings.find(m_FinalPassType)->second);

        // Search from back to front
        
    }
}