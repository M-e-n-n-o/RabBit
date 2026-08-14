#ifdef RB_STEAM_API
#include "RabBitCommon.h"
#include "SteamPlatformService.h"

#include <steam/steam_api.h>

namespace RB
{
    void SteamWarningCallback(int severity, const char* msg)
    {
        // if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
        // if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent

        if (severity == 0)
            RB_LOG(LOGTAG_MAIN, "Steam message: %s", msg)
        else if (severity >= 1)
            RB_LOG_WARN(LOGTAG_MAIN, "Steam warning: %s", msg)
    }

    SteamPlatformService::SteamPlatformService()
        : m_Initialized(false)
    {
        SteamErrMsg err = {};
        ESteamAPIInitResult result = SteamAPI_InitEx(&err);
        if (result != k_ESteamAPIInitResult_OK)
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Failed to initialize Steam API with error %d, and message: %s", result, err);
            return;
        }

        if (!SteamUser()->BLoggedOn())
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Steam user is not logged in, Steam API will not initialize");
            SteamAPI_Shutdown();
            return;
        }

        SteamClient()->SetWarningMessageHook(&SteamWarningCallback);

        m_Initialized = true;
    }

    SteamPlatformService::~SteamPlatformService()
    {
        if (!m_Initialized)
            return;

        SteamAPI_Shutdown();
    }

    void SteamPlatformService::Update()
    {
        if (!m_Initialized)
            return;

        SteamAPI_RunCallbacks();
    }
}
#endif