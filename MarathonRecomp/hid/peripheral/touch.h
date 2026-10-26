#pragma once

#include <hid/peripheral/peripheral.h>

namespace HID
{
    // On-screen touch controls (Android). Attached and detached together with the
    // keyboard and mouse, so it follows the keyboard's user and never takes the
    // free user slot a newly connected gamepad is assigned to.
    class Touch : public Peripheral
    {
    public:
        HID_MAKE_PERIPHERAL_ID(3);

        virtual std::string GetName() const override
        {
            return "Touch";
        }
    };
}
