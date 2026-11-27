#include <api/Marathon.h>
#include <hid/hid.h>
#include <ui/black_bar.h>
#include <user/config.h>
#include <user/achievement_manager.h>
#include <app.h>

void UnlockAchievement(PPCRegister& id)
{
    AchievementManager::Unlock(id.u32);
}

void SetLifeBarAnimation(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5, PPCRegister& r31)
{
    static char* s_lifeBarSceneName{};
    static bool s_contextualColoursApplied{};

    if (!s_lifeBarSceneName)
    {
        constexpr const char* LIFE_BAR_ANIME = "life_bar_anime";
        s_lifeBarSceneName = (char*)g_userHeap.Alloc(strlen(LIFE_BAR_ANIME) + 1);
        strcpy(s_lifeBarSceneName, LIFE_BAR_ANIME);
    }

    const bool useContextualColours = Config::RestoreContextualHUDColours;
    if (s_contextualColoursApplied != useContextualColours)
    {
        // Update the palette when the HUD is rebuilt after a setting change.
        const float shadowColour = useContextualColours ? 1.0f : 0.0f;
        const float silverColour = useContextualColours ? 2.0f : 0.0f;
        auto& colours = Sonicteam::Globals::ms_apMainDisplayColours;
        *colours[Sonicteam::Character_Shadow] = shadowColour;
        *colours[Sonicteam::Character_Omega] = shadowColour;
        *colours[Sonicteam::Character_Rouge] = shadowColour;
        *colours[Sonicteam::Character_Silver] = silverColour;
        *colours[Sonicteam::Character_Amy] = silverColour;
        *colours[Sonicteam::Character_Blaze] = silverColour;
        s_contextualColoursApplied = useContextualColours;
    }

    auto base = g_memory.base;
    auto pCsdObject = (Sonicteam::CsdObject*)(base + PPC_LOAD_U32(r3.u32));
    auto pSceneName = (const char*)(base + r4.u32);

    // Redirect "life_ber_anime" to "life_bar_anime", as they
    // actually spelt "bar" correctly in the tag XNCP scene names...
    if ((pCsdObject->m_pCsdResource->m_MgrResourceName == "sprite/tagdisplay_1p" ||
        pCsdObject->m_pCsdResource->m_MgrResourceName == "sprite/tagdisplay_2p") &&
        strcmp(pSceneName, "life_ber_anime") == 0)
    {
        r4.u32 = g_memory.MapVirtual(s_lifeBarSceneName);
    }

    if (!useContextualColours)
        return;

    auto pHUDMainDisplay = (Sonicteam::HUDMainDisplay*)g_memory.Translate(r31.u32);

    switch (pHUDMainDisplay->m_Character)
    {
        case Sonicteam::Character_Sonic:
        case Sonicteam::Character_Tails:
        case Sonicteam::Character_Knuckles:
            r5.u32 = 0x82036778; // "sonic_in"
            break;

        case Sonicteam::Character_Shadow:
        case Sonicteam::Character_Omega:
        case Sonicteam::Character_Rouge:
            r5.u32 = 0x8203676C; // "shadow_in"
            break;

        case Sonicteam::Character_Silver:
        case Sonicteam::Character_Amy:
        case Sonicteam::Character_Blaze:
            r5.u32 = 0x82036760; // "silver_in"
            break;
    }
}

void SetRingBarIndex(PPCRegister& r5, PPCRegister& r31)
{
    if (!Config::RestoreContextualHUDColours)
        return;

    auto pHUDMainDisplay = (Sonicteam::HUDMainDisplay*)g_memory.Translate(r31.u32);

    r5.u32 = pHUDMainDisplay->m_Character;
}

// Redirects 2P HUD to 1P HUD to remove overscan compensation.
void Load2PDisplayMidAsmHook() {}

void PostureDisableEdgeGrabLeftover(PPCRegister& posture)
{
    if (!Config::DisableEdgeGrabLeftover)
        return;

    auto base = g_memory.base;

    *(volatile uint8_t*)(base + (posture.u32 + 0x3C0)) = 1;
}

void PedestrianAnimationLOD(PPCRegister& val)
{
    if (Config::DisablePedestrianAnimationLOD)
        val.u32 = 0;
}

// The game does some checks for the spatial graph vertex to ensure it's
// free of physics objects or other townsmen before committing to doing
// camera occlusion checks. This skips those checks if the vertex is not
// within the Kynapse view cone.
bool CTownsKeeper_SkipVertexIfOccluded(PPCRegister& r30, PPCRegister& r1)
{
    const auto pKynapseRandomCreator = reinterpret_cast<Sonicteam::KynapseRandomCreator*>(g_memory.Translate(r30.u32));
    const auto pVertex = reinterpret_cast<Kaim::CVector*>(g_memory.Translate(r1.u32 + 0x50));

    return GuestToHostFunction<bool>(sub_8251DD00, pKynapseRandomCreator, pVertex);
}

// When spawning new townsmen, the game iterates every sixth vertex in the spatial
// graph, and checks it for the following:
//
// 1. Is there a physics object at this vertex?
// 2. Is there a townsman at this vertex?
// 3. Is this vertex visible to the camera?
// 4. Is this vertex within 10 metres ahead of the camera inside a 40 degree view cone?
//
// The previous hook reorders the last check so that it happens first, reducing the
// amount of entity checks by a small amount.
//
// The remaining vertices continue on and encounter the third check, which is quite
// expensive, as it delegates this check to the physics engine, rather than checking
// the view cone first.
//
// This hook moves the camera visible test last, so that vertices only do this check
// when within range. The original call to this function is skipped.
bool CTownsKeeper_SkipIntersectIfOccluded(PPCRegister& r30, PPCRegister& r5)
{
    const auto pKynapseRandomCreator = reinterpret_cast<Sonicteam::KynapseRandomCreator*>(g_memory.Translate(r30.u32));
    const auto pKyCameraEntity = pKynapseRandomCreator->m_pKyCameraEntity.get();

    struct Query
    {
        xpointer<Kaim::CWorld> pWorld{};
        be<float> X{};
        be<float> Y{};
    };

    auto query = guest_stack_var<Query>();
    query->pWorld = pKyCameraEntity->m_owner;
    query->X = pKyCameraEntity->m_position.X;
    query->Y = pKyCameraEntity->m_position.Y;

    return GuestToHostFunction<bool>(sub_8251E358, pKynapseRandomCreator, query.get(), r5.u32);
}

// Sonicteam::CommonObjectHint::Update
PPC_FUNC_IMPL(__imp__sub_822CE930);
PPC_FUNC(sub_822CE930)
{
    auto* pCommonObjectHint = (Sonicteam::CommonObjectHint*)(base + ctx.r3.u32);
    auto* pGame = App::s_pApp->m_pDoc->GetDocMode<Sonicteam::GameMode>()->GetGame();
    auto& rMessageName = pCommonObjectHint->m_MessageName;

    if (!Config::Hints && pCommonObjectHint->m_Type == Sonicteam::CommonObjectHint::CommonObjectHintType_HintRing)
    {
        pCommonObjectHint->Destroy();
        return;
    }

    if (!Config::ControlTutorial || Config::SlidingAttack != ESlidingAttack::X)
    {
        // Get global flag for Sonic's Antigravity being unlocked to remove "hint_all03_h26_so".
        guest_stack_var<Sonicteam::Message::Mission::MsgGetGlobalFlag> msgGetSonicAntigravityFlag(6001);
        pGame->m_pMissionCore->ProcessMessage(msgGetSonicAntigravityFlag.get());

        if (msgGetSonicAntigravityFlag->FlagValue != 0 && strcmp(rMessageName, "hint_twn01_e02_tl") == 0)
        {
            pCommonObjectHint->Destroy();
            return;
        }
    }

    if (!Config::ControlTutorial || Config::LightDash != ELightDash::X)
    {
        // Get global flag for Sonic's Light Dash being unlocked to remove "hint_twn01_e01_tl".
        guest_stack_var<Sonicteam::Message::Mission::MsgGetGlobalFlag> msgGetSonicLightDashFlag(6000);
        pGame->m_pMissionCore->ProcessMessage(msgGetSonicLightDashFlag.get());

        // Get global flag for Shadow's Light Dash being unlocked to remove "hint_twn01_e44_rg".
        guest_stack_var<Sonicteam::Message::Mission::MsgGetGlobalFlag> msgGetShadowLightDashFlag(6012);
        pGame->m_pMissionCore->ProcessMessage(msgGetShadowLightDashFlag.get());

        auto isSonicLightDashHint = msgGetSonicLightDashFlag->FlagValue != 0 && strcmp(rMessageName, "hint_twn01_e00_tl") == 0;
        auto isShadowLightDashHint = msgGetShadowLightDashFlag->FlagValue != 0 && strcmp(rMessageName, "hint_twn01_e43_rg") == 0;

        if (isSonicLightDashHint || isShadowLightDashHint ||
            strcmp(rMessageName, "hint_all03_h00_so") == 0 ||
            strcmp(rMessageName, "hint_all03_h31_so") == 0)
        {
            pCommonObjectHint->Destroy();
            return;
        }
    }

    const auto isPlayStation = Config::GetPreferredControllerIcons() == EControllerIcons::PlayStation;

    if (!Config::ControlTutorial || isPlayStation)
    {
        guest_stack_var<int> stack{};

        auto pspTextCard = GuestToHostFunction<boost::shared_ptr<Sonicteam::TextCard>*>(sub_825ECB48,
            stack.get(), App::s_pApp->GetGame()->m_pHintTextBook.get(), (const char*)rMessageName);

        if (auto pTextCard = pspTextCard->get())
        {
            if (pTextCard->m_pVariables)
            {
                if (!Config::ControlTutorial)
                {
                    if (strstr(pTextCard->m_pVariables, "picture(button_"))
                    {
                        pCommonObjectHint->Destroy();
                        return;
                    }
                }

                if (isPlayStation)
                {
                    // L1/R1 and L2/R2 are flipped on PS3, leading
                    // to the voice lines being wrong for the hints.
                    // 
                    // We'll provide an alternate control scheme to
                    // address this later on, but for now these should
                    // be hidden.

                    if (strstr(pTextCard->m_pVariables, "button_lb") || strstr(pTextCard->m_pVariables, "button_rb"))
                    {
                        pCommonObjectHint->Destroy();
                        return;
                    }
                }
            }
        }
    }

    __imp__sub_822CE930(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_8244D288);
PPC_FUNC(sub_8244D288)
{
    auto isShadowEggCerberus = PPC_LOAD_U32(ctx.r3.u32 + 0x1C);

    // Prevent Eggman's voice line playing
    // for Shadow's variant of Egg Cerberus.
    if (!Config::Hints && isShadowEggCerberus)
        return;

    __imp__sub_8244D288(ctx, base);
}

bool Super3_DisableChangeRequestHint()
{
    return !Config::ControlTutorial;
}

PPC_FUNC_IMPL(__imp__sub_824A6EA8);
PPC_FUNC(sub_824A6EA8)
{
    if ((App::s_isSkipLogos || Config::SkipIntroLogos) && ctx.r4.u32 == 1)
        ctx.r4.u32 = 4;

    __imp__sub_824A6EA8(ctx, base);
}

PPC_FUNC(sub_82188460)
{
    ctx.r3.u64 = Config::Subtitles;
}

void NOP() {}
