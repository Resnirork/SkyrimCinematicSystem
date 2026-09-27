#include "CameraHook.h"
#include "CameraRuntime.h"

namespace CinematicSystem::Hook {
namespace {
using FreeCameraUpdateFn = void (*)(RE::FreeCameraState*, RE::BSTSmartPointer<RE::TESCameraState>&);
constexpr std::size_t kFreeCameraVTableSize = 9;
constexpr std::size_t kFreeCameraUpdateSlot = 3;
std::array<std::uintptr_t, kFreeCameraVTableSize + 1> g_freeCameraVTable{};
std::uintptr_t* g_originalVTable = nullptr;
FreeCameraUpdateFn g_originalFreeCameraUpdate = nullptr;
RE::FreeCameraState* g_hookedState = nullptr;

void UpdateFreeCamera(RE::FreeCameraState* a_state, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
{
    if (g_originalFreeCameraUpdate) {
        g_originalFreeCameraUpdate(a_state, a_nextState);
    }

    Runtime::GetSingleton().OnCameraUpdate(a_state->camera);
}
}  // namespace

bool Install(RE::FreeCameraState* state)
{
    if (!state) {
        logs::error("cannot hook a null FreeCameraState.");
        return false;
    }
    if (g_hookedState == state) return true;
    if (g_hookedState) Uninstall(g_hookedState);

    auto** vtable = reinterpret_cast<std::uintptr_t**>(state);
    g_originalVTable = *vtable;
    g_freeCameraVTable[0] = g_originalVTable[-1];
    std::copy_n(g_originalVTable, kFreeCameraVTableSize, g_freeCameraVTable.begin() + 1);
    g_originalFreeCameraUpdate = reinterpret_cast<FreeCameraUpdateFn>(g_freeCameraVTable[kFreeCameraUpdateSlot + 1]);
    if (!g_originalFreeCameraUpdate) {
        g_originalVTable = nullptr;
        logs::error("FreeCameraState::Update original is null; state hook not installed.");
        return false;
    }

    g_freeCameraVTable[kFreeCameraUpdateSlot + 1] = reinterpret_cast<std::uintptr_t>(&UpdateFreeCamera);
    *vtable = g_freeCameraVTable.data() + 1;
    g_hookedState = state;
    logs::info("FreeCameraState::Update hook installed on active state.");
    return true;
}

void Uninstall(RE::FreeCameraState* state)
{
    if (!state || g_hookedState != state) return;

    auto** vtable = reinterpret_cast<std::uintptr_t**>(state);
    *vtable = g_originalVTable;
    g_hookedState = nullptr;
    g_originalVTable = nullptr;
    g_originalFreeCameraUpdate = nullptr;
    logs::debug("FreeCameraState::Update hook removed.");
}

}  // namespace CinematicSystem::Hook
