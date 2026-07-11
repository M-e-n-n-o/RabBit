#pragma once

namespace RB::Tools
{
    constexpr uint32_t CreateMagic(char a, char b, char c, char d)
    {
        return (static_cast<uint32_t>(static_cast<uint8_t>(a))) |
               (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
               (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
               (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
    }
}