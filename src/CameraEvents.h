#pragma once
#include "pch.h"

namespace CinematicSystem::Events {

bool RegisterPapyrusEvents(RE::BSScript::IVirtualMachine* vm);
bool Register(RE::TESForm* form);
bool Unregister(RE::TESForm* form);
void ScriptFinished(std::string_view name, bool cancelled);

}  // namespace CinematicSystem::Events
