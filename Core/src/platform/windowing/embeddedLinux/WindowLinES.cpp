#if RB_PLATFORM_LINUX_ES

#include "RabBitCommon.h"
#include "WindowLinES.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> 

namespace RB::Graphics::LinuxES
{
    WindowLinuxES::WindowLinuxES(const WindowArgs& args)
        : Window(true, args.virtualScale, args.virtualAspect)
    {
        m_DrmFileDescriptor = open(args.drmDeviceName, O_RDWR | O_CLOEXEC);
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