#pragma once

#include <Marathon.inl>

namespace Sonicteam::Mission
{
    class VirtualMachine;

    class Core : public SoX::MessageReceiver
    {
    public:
        MARATHON_INSERT_PADDING(0x0C);
        xpointer<VirtualMachine> m_pVirtualMachine;
        MARATHON_INSERT_PADDING(0xC80C);
    };

    MARATHON_ASSERT_OFFSETOF(Core, m_pVirtualMachine, 0x10);
    MARATHON_ASSERT_SIZEOF(Core, 0xC820);
}
