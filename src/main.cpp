#include "pch.h"
#include "CameraRuntime.h"
#include "CinematicSystemPapyrus.h"
#include "CameraHook.h"
#include "Config.h"

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    CinematicSystem::Config::Load();
    logs::info("loading");

    if (const auto papyrus = SKSE::GetPapyrusInterface()) {
        if (!papyrus->Register(CinematicSystem::Papyrus::Register)) {
            logs::error("failed to register Papyrus functions");
            return false;
        }
    } else {
        logs::error("Papyrus interface unavailable");
        return false;
    }

    CinematicSystem::Runtime::GetSingleton().Initialize();
    logs::info("loaded");
    return true;
}
