#include "RabBitCommon.h"
#include "PlatformService.h"

#ifdef RB_STEAM_API
#include "platform/platformService/steamworks/SteamPlatformService.h"
#include "platform/platformService/steamworks/SteamNetworkService.h"
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
            break;
        }
        return nullptr;
    }

    PlatformNetworkService* PlatformNetworkService::Create()
    {
        switch (PlatformService::GetAPI())
        {
#ifdef RB_STEAM_API
        case RB::PlatformAPI::Steamworks:
            return new SteamNetworkService();
#endif
        default:
            // TODO: Create a local network service for testing purposes
            RB_LOG_WARN(LOGTAG_MAIN, "Invalid network service integration API");
            break;
        }
        return nullptr;
    }
}
