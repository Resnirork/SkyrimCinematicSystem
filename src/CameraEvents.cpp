#include "CameraEvents.h"

namespace CinematicSystem::Events {

// CommonLibSSE-NG's RegistrationSet stores Papyrus VM handles and provides
// SendEvent/QueueEvent with typed arguments.
static SKSE::RegistrationSet<std::string, bool> registrations{"OnScriptFinishedEvent"};

bool RegisterPapyrusEvents(RE::BSScript::IVirtualMachine*)
{
    return true;
}

bool Register(RE::TESQuest* quest)
{
    if (!quest) {
        logs::warn("CinematicSystem: cannot register script-finished events for a null quest.");
        return false;
    }
    const bool registered = registrations.Register(quest);
    if (!registered) logs::warn("CinematicSystem: failed to register script-finished events.");
    return registered;
}

bool Unregister(RE::TESQuest* quest)
{
    if (!quest) {
        logs::warn("CinematicSystem: cannot unregister script-finished events for a null quest.");
        return false;
    }
    const bool unregistered = registrations.Unregister(quest);
    if (!unregistered) logs::debug("CinematicSystem: script-finished event registration was not found.");
    return unregistered;
}

void ScriptFinished(std::string_view name, bool cancelled)
{
    registrations.QueueEvent(std::string(name), cancelled);
}

}  // namespace CinematicSystem::Events
