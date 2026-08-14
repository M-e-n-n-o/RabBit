#include "RabBitCommon.h"
#include "PlatformService.h"

#ifdef RB_STEAM_API
#include "platform/platformService/steamworks/SteamPlatformService.h"
#include "platform/platformService/steamworks/SteamNetworkService.h"
#endif

#ifdef RB_PLATFORM_WINDOWS
#include "platform/platformService/windows/WindowsNetworkService.h"
#endif

namespace RB
{
    PlatformService* PlatformService::Create(PlatformAPI api)
    {
        s_Api = api;
        switch (api)
        {
#ifdef RB_STEAM_API
        case RB::PlatformAPI::Steamworks:
            RB_LOG(LOGTAG_MAIN, "Platform integration API: Steamworks");
            return new SteamPlatformService();
#endif
        default:
            RB_LOG_WARN(LOGTAG_MAIN, "Invalid platform integration API");
            return nullptr;
        }
    }

    PlatformNetworkService* PlatformNetworkService::Create()
    {
        switch (PlatformService::GetAPI())
        {
#ifdef RB_STEAM_API
        case RB::PlatformAPI::Steamworks:
            RB_LOG(LOGTAG_MAIN, "Network platform integration API: Steamworks");
            return new SteamNetworkService();
#endif
        default:
#ifdef RB_PLATFORM_WINDOWS
            RB_LOG(LOGTAG_MAIN, "Network platform integration API: Winsock");
            return new WindowsNetworkService();
#else
            RB_LOG_WARN(LOGTAG_MAIN, "Invalid network service integration API");
            return nullptr;
#endif
        }
    }
}
