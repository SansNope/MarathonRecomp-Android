#pragma once

#include <Marathon.inl>
#include <Sonicteam/SoX/Array.h>
#include <Sonicteam/SoX/MessageReceiver.h>
#include <stdx/string.h>

namespace Sonicteam::Mission
{
    class VirtualMachine : public SoX::MessageReceiver
    {
    public:
        MARATHON_INSERT_PADDING(0x04);
        stdx::string m_MissionName;
        MARATHON_INSERT_PADDING(0x1A0);
        SoX::Array<be<int>, 64> m_TemporaryFlags;
        MARATHON_INSERT_PADDING(0x24C);
    };

    MARATHON_ASSERT_OFFSETOF(VirtualMachine, m_MissionName, 0x08);
    MARATHON_ASSERT_OFFSETOF(VirtualMachine, m_TemporaryFlags, 0x1C4);
    MARATHON_ASSERT_SIZEOF(VirtualMachine, 0x510);
}
