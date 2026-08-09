#pragma once
#ifdef RB_STEAM_API

#include "app/PlatformService.h"

#include <steam/steam_api.h>

namespace RB
{
    class SteamPlatformService : public PlatformService
    {
    public:
        SteamPlatformService();
        ~SteamPlatformService();

        bool IsInitialized() const override { return m_Initialized; }

        void Update() override;

    private:
        bool m_Initialized;
    };
}
#endif