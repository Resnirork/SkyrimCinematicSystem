#pragma once

#include <RE/Skyrim.h>

namespace CinematicSystem::CameraTransform {

struct Pose {
    RE::NiPoint3 position{};
    float pitch{0.0f};
    float yaw{0.0f};
    float worldFOV{0.0f};
    float firstPersonFOV{0.0f};
};

bool Capture(RE::PlayerCamera* camera, RE::FreeCameraState* freeState, Pose& pose);
void ApplyToFreeCamera(RE::FreeCameraState* freeState, const Pose& pose);
void Restore(RE::PlayerCamera* camera, RE::FreeCameraState* freeState, const Pose& pose);

}  // namespace CinematicSystem::CameraTransform