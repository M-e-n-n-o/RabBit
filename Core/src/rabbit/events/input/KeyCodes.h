#pragma once

namespace RB::Events
{
    enum class KeyCode
    {
        // Control keys
        Enter = 13,        // 0x0D (VK_RETURN)
        Space = 32,        // 0x20 (VK_SPACE)

        // Alphanumeric keys directly match uppercase ASCII
        A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
        I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
        Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
        Y = 89, Z = 90,

        // Function keys
        F1 = 112,          // 0x70 (VK_F1)
        F2 = 113,          // 0x71 (VK_F2)
        F3 = 114,          // 0x72 (VK_F3)
        F4 = 115,          // 0x73 (VK_F4)
        F5 = 116,          // 0x74 (VK_F5)
        F6 = 117,          // 0x75 (VK_F6)
        F7 = 118,          // 0x76 (VK_F7)
        F8 = 119,          // 0x77 (VK_F8)
        F9 = 120,          // 0x78 (VK_F9)
        F10 = 121,         // 0x79 (VK_F10)
        F11 = 122,         // 0x7A (VK_F11)
        F12 = 123,         // 0x7B (VK_F12)

        // Modifier keys
        LeftShift = 160,   // 0xA0 (VK_LSHIFT)
        RightShift = 161,  // 0xA1 (VK_RSHIFT)
        LeftControl = 162, // 0xA2 (VK_LCONTROL)
        RightControl = 163,// 0xA3 (VK_RCONTROL)
        LeftAlt = 164,     // 0xA4 (VK_LMENU)
        RightAlt = 165,    // 0xA5 (VK_RMENU)
    };
}