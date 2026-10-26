#pragma once

#include <hid/peripheral/touch.h>
#include <ui/touch_controls.h>

namespace HID
{
    class TouchSDL : public Touch
    {
    public:
        virtual XAMINPUT_GAMEPAD GetState() override
        {
            m_State = TouchControls::IsVisible()
                ? TouchControls::GetGamepadState()
                : XAMINPUT_GAMEPAD{};

            return m_State;
        }
    };
}
