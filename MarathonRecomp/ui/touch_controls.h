#pragma once

// Ported from UnleashedRecomp-Android 0.6.1 (ui/touch_controls.h).
//
// On-screen touch controls (Android).
//
// Renders a virtual gamepad (left analog stick + face/shoulder/trigger/start-back
// buttons) as an ImGui overlay and synthesises an XAMINPUT_GAMEPAD state that the
// HID layer reports for the user that owns the keyboard (player 1 by default)
// through HID::TouchSDL, merged with the other peripherals of that user.
//
// In Auto mode, visibility follows the active input source: visible by default,
// hidden as soon as real gamepad input is received, and shown again on the next
// screen touch. The Android
// Touch Controls setting can override that state with Always On or Off.

class TouchControls
{
public:
    // Whether the overlay is currently shown AND feeding input. When false, the
    // HID layer falls back to any physical controller.
    static bool IsVisible();

    // Update Auto mode's visibility. Called with false from the HID layer when
    // a physical controller reports input; called with true internally on touch.
    // Always On and Off take precedence without losing the remembered Auto state.
    static void SetVisible(bool visible);

    // Latest synthesised pad state. Valid only while IsVisible().
    static XAMINPUT_GAMEPAD GetGamepadState();

    // Game-context signals for the adaptive layout. NotifyMenuVisible must be
    // called every frame a navigation menu is on screen (the flag decays on its
    // own); the cutscene flag is edge-triggered from scene ctor/dtor hooks
    // (Sonic 2006 hooks for these are not wired up yet);
    // NotifyMovieVisible is stamped per rendered WMV frame (attract movie, opening
    // and the like) and decays like the menu flag. In a menu the left stick becomes
    // a D-pad; in a cutscene or movie everything collapses to a single SKIP button.
    static void NotifyMenuVisible();
    static void NotifyCutsceneActive(bool active);
    static void NotifyMovieVisible();

    static void Init();
    static void Draw();
};
