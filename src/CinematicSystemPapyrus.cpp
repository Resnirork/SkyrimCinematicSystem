#include "CinematicSystemPapyrus.h"
#include "CameraEvents.h"
#include "CameraRuntime.h"

namespace CinematicSystem::Papyrus {

using Runtime = CinematicSystem::Runtime;

void FindFiles(RE::StaticFunctionTag*) { Runtime::GetSingleton().EnumerateScripts(); }
std::int32_t GetFileCount(RE::StaticFunctionTag*) { return static_cast<std::int32_t>(Runtime::GetSingleton().GetFilenames().size()); }
std::string GetFilename(RE::StaticFunctionTag*, std::int32_t index)
{
    const auto files = Runtime::GetSingleton().GetFilenames();
    if (index < 0 || static_cast<std::size_t>(index) >= files.size()) return {};
    return files[static_cast<std::size_t>(index)];
}

bool StartScript(RE::StaticFunctionTag*, std::string filename, bool hideInterface = false)
{
    return Runtime::GetSingleton().Start(std::move(filename), hideInterface);
}
void StopScript(RE::StaticFunctionTag*) { Runtime::GetSingleton().Stop(true); }
bool IsScriptRunning(RE::StaticFunctionTag*) { return Runtime::GetSingleton().IsRunning(); }

bool StartRecording(RE::StaticFunctionTag*) { return Runtime::GetSingleton().StartRecording(); }
bool StopRecording(RE::StaticFunctionTag*) { return Runtime::GetSingleton().StopRecording(); }
bool IsScriptRecording(RE::StaticFunctionTag*) { return Runtime::GetSingleton().IsRecording(); }
bool SaveRecordingPoint(RE::StaticFunctionTag*, bool absolute) { return Runtime::GetSingleton().SaveRecordingPoint(absolute); }

float GetCurrentPosX(RE::StaticFunctionTag*) { return Runtime::GetSingleton().PosX(); }
float GetCurrentPosY(RE::StaticFunctionTag*) { return Runtime::GetSingleton().PosY(); }
float GetCurrentPosZ(RE::StaticFunctionTag*) { return Runtime::GetSingleton().PosZ(); }
float GetCurrentRotX(RE::StaticFunctionTag*) { return Runtime::GetSingleton().RotX(); }
float GetCurrentRotZ(RE::StaticFunctionTag*) { return Runtime::GetSingleton().RotZ(); }
float GetCurrentFOV(RE::StaticFunctionTag*) { return Runtime::GetSingleton().FOV(); }
float GetCurrentTimeMultiplier(RE::StaticFunctionTag*) { return Runtime::GetSingleton().TimeMultiplier(); }

bool RegisterForCinematicScriptEvents(RE::StaticFunctionTag*, RE::TESForm* form)
{
    const bool newlyRegistered = Events::Register(form);
    logs::info("Papyrus form event registration requested; newly registered: {}.", newlyRegistered);
    return newlyRegistered;
}
bool UnregisterForCinematicScriptEvents(RE::StaticFunctionTag*, RE::TESForm* form) { return Events::Unregister(form); }

bool Register(RE::BSScript::IVirtualMachine* vm)
{
    if (!vm) {
        logs::error("cannot register Papyrus functions without a virtual machine.");
        return false;
    }
    vm->RegisterFunction("FindFiles", "SkyrimCinematicSystem", FindFiles);
    vm->RegisterFunction("GetFileCount", "SkyrimCinematicSystem", GetFileCount);
    vm->RegisterFunction("GetFilename", "SkyrimCinematicSystem", GetFilename);
    vm->RegisterFunction("StartScript", "SkyrimCinematicSystem", StartScript);
    vm->RegisterFunction("StopScript", "SkyrimCinematicSystem", StopScript);
    vm->RegisterFunction("IsScriptRunning", "SkyrimCinematicSystem", IsScriptRunning);
    vm->RegisterFunction("RegisterForCinematicScriptEvents", "SkyrimCinematicSystem", RegisterForCinematicScriptEvents);
    vm->RegisterFunction("UnregisterForCinematicScriptEvents", "SkyrimCinematicSystem", UnregisterForCinematicScriptEvents);
    vm->RegisterFunction("StartRecording", "SkyrimCinematicSystem", StartRecording);
    vm->RegisterFunction("StopRecording", "SkyrimCinematicSystem", StopRecording);
    vm->RegisterFunction("IsScriptRecording", "SkyrimCinematicSystem", IsScriptRecording);
    vm->RegisterFunction("SaveRecordingPoint", "SkyrimCinematicSystem", SaveRecordingPoint);
    vm->RegisterFunction("GetCurrentPosX", "SkyrimCinematicSystem", GetCurrentPosX);
    vm->RegisterFunction("GetCurrentPosY", "SkyrimCinematicSystem", GetCurrentPosY);
    vm->RegisterFunction("GetCurrentPosZ", "SkyrimCinematicSystem", GetCurrentPosZ);
    vm->RegisterFunction("GetCurrentRotX", "SkyrimCinematicSystem", GetCurrentRotX);
    vm->RegisterFunction("GetCurrentRotZ", "SkyrimCinematicSystem", GetCurrentRotZ);
    vm->RegisterFunction("GetCurrentFOV", "SkyrimCinematicSystem", GetCurrentFOV);
    vm->RegisterFunction("GetCurrentTimeMultiplier", "SkyrimCinematicSystem", GetCurrentTimeMultiplier);
    logs::debug("Papyrus functions registered.");
    return true;
}

}  // namespace CinematicSystem::Papyrus
