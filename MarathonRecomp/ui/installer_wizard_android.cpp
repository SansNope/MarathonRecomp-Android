#include "installer_wizard.h"
#include <os/logger.h>
#include <ui/game_window.h>

// The desktop wizard picks game sources through the native file dialog, which has
// no Android counterpart usable with std::filesystem. Until the Java launcher
// imports game data, tell the user where the extracted files are expected.

void InstallerWizard::Init()
{
}

void InstallerWizard::Draw()
{
}

void InstallerWizard::Shutdown()
{
}

bool InstallerWizard::Run(std::filesystem::path installPath, bool skipGame)
{
    const auto gamePath = (installPath / "game").string();

    LOGFN_ERROR("Game files were not found or are incomplete. Expected default.xex in: {}", gamePath);

    const auto message = fmt::format(
        "Sonic the Hedgehog (2006) game files were not found.\n\n"
        "Copy the extracted Xbox 360 game files (the folder containing default.xex) to:\n{}",
        gamePath);

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), message.c_str(), GameWindow::s_pWindow);

    return false;
}
