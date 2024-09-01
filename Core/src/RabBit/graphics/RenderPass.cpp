#include "RabBitCommon.h"
#include "RenderPass.h"

namespace RB::Graphics
{
    // ---------------------------------------------------------------------------
    //								RenderPassConfig
    // ---------------------------------------------------------------------------

    RenderPassConfigBuilder::RenderPassConfigBuilder(const RenderPassType& type, const char* friendly_name, bool only_execute_when_depended_on)
    {
        m_Config = {};
        m_Config.type = type;
        m_Config.friendlyName = friendly_name;
        m_Config.onlyExecuteWhenDependedOn = only_execute_when_depended_on;

        m_TotalDependencies = 0;
        for (int i = 0; i < _countof(m_Config.dependencies); i++)
        {
            m_Config.dependencies[i] = RenderPassType::None;
        }
    }

    RenderPassConfigBuilder& RenderPassConfigBuilder::AddDependency(RenderPassType type)
    {
        m_Config.dependencies[m_TotalDependencies] = type;
        m_TotalDependencies++;

        return *this;
    }

    RenderPassConfig RenderPassConfigBuilder::Build()
    {
        return m_Config;
    }

    // ---------------------------------------------------------------------------
    //									RenderPass
    // ---------------------------------------------------------------------------
}