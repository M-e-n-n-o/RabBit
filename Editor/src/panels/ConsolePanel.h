#pragma once

#include <RabBit.h>
#include "WindowPanel.h"

namespace Editor
{
    class ConsolePanel : public WindowPanel
    {
    public:
        ConsolePanel();

        void OnCreate() override;
        void OnDestroy() override;
        void OnUpdate() override;

        void AppendLog(int mode, const char* text);

    private:
        const static uint32_t c_MaxCharacters;
        std::string m_Text;
    };
}