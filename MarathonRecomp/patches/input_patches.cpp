#include "input_patches.h"
#include <api/Marathon.h>
#include <user/config.h>
#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace
{
    constexpr double kBHoldSeconds = 0.25;

    std::mutex g_bHoldMutex;
    std::unordered_map<uint32_t, double> g_bHoldTimes;

    void ClearBHold(uint32_t listenerAddr)
    {
        std::lock_guard lock(g_bHoldMutex);
        g_bHoldTimes.erase(listenerAddr);
    }

    uint32_t UpdateBHold(uint32_t listenerAddr, bool pressed, bool down, bool released, double dt)
    {
        std::lock_guard lock(g_bHoldMutex);
        if (pressed)
            g_bHoldTimes.insert_or_assign(listenerAddr, 0.0);

        const auto it = g_bHoldTimes.find(listenerAddr);
        if (it == g_bHoldTimes.end())
            return 0u;

        const double elapsed = pressed ? 0.0 : std::min(it->second + std::max(0.0, dt), kBHoldSeconds);
        if (down && !released)
        {
            it->second = elapsed;
            return 0u;
        }

        g_bHoldTimes.erase(it);
        if (!released)
            return 0u;

        return elapsed >= kBHoldSeconds ? InputListenerButtons::BHoldReleased : InputListenerButtons::BTapReleased;
    }
}

// Sonicteam::Player::Input::ListenerNormal::~ListenerNormal
PPC_FUNC_IMPL(__imp__sub_82222348);
PPC_FUNC(sub_82222348)
{
    ClearBHold(ctx.r3.u32);
    __imp__sub_82222348(ctx, base);
}

// Sonicteam::Player::Input::ListenerNormal::Update
PPC_FUNC_IMPL(__imp__sub_82222428);
PPC_FUNC(sub_82222428)
{
    const uint32_t listenerAddr = ctx.r3.u32;
    const double dt = ctx.f1.f64;
    auto* pListenerNormal = reinterpret_cast<Sonicteam::Player::Input::ListenerNormal*>(base + listenerAddr);
    auto* pInputManager = reinterpret_cast<Sonicteam::SoX::Input::Manager*>(base + ctx.r4.u32);

    __imp__sub_82222428(ctx, base);

    if (!*pListenerNormal->m_pIsListening)
    {
        ClearBHold(listenerAddr);
        return;
    }

    const auto& padState = pInputManager->PadState;
    constexpr auto east = Sonicteam::SoX::Input::PadButton_East;
    const bool down = padState.IsDown(east);
    const bool pressed = padState.IsPressed(east);
    uint32_t buttons = pListenerNormal->m_State.get();
    if (down)
        buttons |= InputListenerButtons::BDown;
    if (pressed)
        buttons |= InputListenerButtons::BPressed;

    if (Config::ImprovedSpindash)
    {
        buttons |= UpdateBHold(listenerAddr, pressed, down, padState.IsReleased(east), dt);

        // The original X pressed bit also fires when the timed action reports a short release.
        const auto actionB = pListenerNormal->m_ActionB.get();
        if (actionB != 0 && padState.IsPressed(actionB))
            buttons |= InputListenerButtons::XPhysicalPressed;
    }
    else
    {
        ClearBHold(listenerAddr);
    }

    pListenerNormal->m_State = buttons;
}

void RemapAntigravityEnter(PPCRegister& r11, PPCRegister& r28)
{
    // Improved Spindash handles the release in Sonic's ground state updates.
    if (Config::ImprovedSpindash)
    {
        r11.u64 = 0;
        return;
    }

    const uint32_t state = Config::SlidingAttack == ESlidingAttack::B ? InputListenerButtons::BDown : 0x800u;
    r11.u64 = (r28.u32 & state) != 0;
}

void RemapAntigravityExit(PPCRegister& r11, PPCRegister& r30)
{
    if (Config::ImprovedSpindash)
    {
        // The release starts the slide. A fresh press can end it early.
        r11.u64 = (r30.u32 & InputListenerButtons::BPressed) != 0;
        return;
    }

    if (Config::SlidingAttack == ESlidingAttack::B)
        r11.u64 = (r30.u32 & InputListenerButtons::BDown) == 0;
}

void RemapLightDash(PPCRegister& r3, PPCRegister& r11)
{
    r11.u64 = ((r3.u32 >> (Config::LightDash == ELightDash::X ? 8 : 15)) & 1) != 0;
}
