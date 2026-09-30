#include "CameraTransform.h"

namespace CinematicSystem::CameraTransform {

bool Capture(RE::PlayerCamera* camera, RE::FreeCameraState* freeState, Pose& pose)
{
    if (!camera) return false;

    const auto& runtimeData = camera->GetRuntimeData2();
    pose.worldFOV = runtimeData.worldFOV;
    pose.firstPersonFOV = runtimeData.firstPersonFOV;

    if (freeState) {
        pose.position = freeState->translation;
        pose.pitch = freeState->rotation.x;
        pose.yaw = freeState->rotation.y;
    } else if (camera->currentState) {
        camera->currentState->GetTranslation(pose.position);
        RE::NiQuaternion rotation;
        camera->currentState->GetRotation(rotation);
        RE::NiPoint3 euler{};
        const bool hasUniqueYaw = rotation.ToRotation().ToEulerAnglesXYZ(euler);
        pose.pitch = euler.x;
        if (hasUniqueYaw) {
            pose.yaw = euler.z;
        } else {
            pose.yaw = runtimeData.yaw;
            logs::warn("camera orientation is near gimbal lock; preserving camera yaw fallback.");
        }
    } else {
        pose.position = runtimeData.pos;
        pose.yaw = runtimeData.yaw;
    }
    return true;
}

void ApplyToFreeCamera(RE::FreeCameraState* freeState, const Pose& pose)
{
    if (!freeState) return;
    freeState->translation = pose.position;
    freeState->rotation.x = pose.pitch;
    freeState->rotation.y = pose.yaw;
}

void Restore(RE::PlayerCamera* camera, RE::FreeCameraState* freeState, const Pose& pose)
{
    if (!camera) return;
    ApplyToFreeCamera(freeState, pose);
    auto& runtimeData = camera->GetRuntimeData2();
    runtimeData.worldFOV = pose.worldFOV;
    runtimeData.firstPersonFOV = pose.firstPersonFOV;
}

}  // namespace CinematicSystem::CameraTransform