#include <stdafx.h>
#include <api/Marathon.h>
#include <app.h>
#include <cstring>

namespace
{
    void FixRadicalTrainMirror(const PPCContext& ctx, const uint8_t* base)
    {
        if (!ctx.r4.u32 || !ctx.r6.u32)
            return;

        const auto* functionName = reinterpret_cast<const char*>(base + ctx.r4.u32);
        if (std::strcmp(functionName, "on_event") != 0)
            return;

        const auto* eventName = reinterpret_cast<const char*>(base + ctx.r6.u32);
        if (std::strcmp(eventName, "goto_rct") != 0)
            return;

        const auto* app = App::s_pApp;
        if (!app || !app->m_pDoc.get())
            return;

        const auto* game = app->GetGame();
        auto* missionCore = game ? game->m_pMissionCore.get() : nullptr;
        auto* missionVm = missionCore ? missionCore->m_pVirtualMachine.get() : nullptr;
        if (!missionVm || missionVm->m_MissionName != "scripts/mission/0007/mission.lua")
            return;

        // The mirror checks c_flag_0507, which the shop sets only when the bracelet
        // is bought in this mission. Global flag 6002 also covers a bracelet
        // obtained earlier or outside the shop.
        guest_stack_var<Sonicteam::Message::Mission::MsgGetGlobalFlag> bounceBracelet(6002);
        missionCore->ProcessMessage(bounceBracelet.get());
        if (bounceBracelet->FlagValue == 1)
            missionVm->m_TemporaryFlags[7] = 1;
    }
}

// Sonicteam::LuaSystem::Callback
PPC_FUNC_IMPL(__imp__sub_8263AEE0);
PPC_FUNC(sub_8263AEE0)
{
    FixRadicalTrainMirror(ctx, base);
    __imp__sub_8263AEE0(ctx, base);
}
