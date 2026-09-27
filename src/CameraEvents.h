#pragma once
#include "pch.h"

namespace CinematicSystem::Events {

bool RegisterPapyrusEvents(RE::BSScript::IVirtualMachine* vm);
bool Register(RE::TESQuest* quest);
bool Unregister(RE::TESQuest* quest);
void ScriptFinished(std::string_view name, bool cancelled);

}  // namespace CinematicSystem::Events
