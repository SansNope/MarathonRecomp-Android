#pragma once

#include <cstdint>

namespace InputListenerButtons
{
    constexpr uint32_t BDown = 0x02000000u;
    constexpr uint32_t BPressed = 0x04000000u;
    constexpr uint32_t BTapReleased = 0x08000000u;
    constexpr uint32_t BHoldReleased = 0x10000000u;
    constexpr uint32_t XPhysicalPressed = 0x20000000u;
}
