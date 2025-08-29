#if RB_PLATFORM_LINUX_ES

#include "RabBitCommon.h"
#include "WindowLinES.h"

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

namespace RB::Graphics::LinuxES
{
    WindowLinuxES::WindowLinuxES(const WindowArgs& args)
        : Window(true, args.virtualScale, args.virtualAspect)
    {
        m_DrmFileDescriptor = open(drmDeviceName, O_RDWR | O_CLOEXEC);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_DrmFileDescriptor >= 0, "Could not open DRM device");

        m_DrmResources = drmModeGetResources(m_DrmFileDescriptor);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_DrmResources != nullptr, "Could not get the DRM resources");

        RB_LOG(LOGTAG_WINDOWING, "Success!");
    }

    WindowLinuxES::~WindowLinuxES()
    {
        if (m_DrmResources)
        {
            drmModeFreeResources(m_DrmResources);
        }

        if (m_DrmFileDescriptor >= 0)
        {
            close(m_DrmFileDescriptor);
        }
    }
}
#endif