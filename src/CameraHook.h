#pragma once
#include "pch.h"

namespace CinematicSystem::Hook {

// Hooks Update on the active FreeCameraState instance.
bool Install(RE::FreeCameraState* state);
void Uninstall(RE::FreeCameraState* state);

}  // namespace CinematicSystem::Hook
