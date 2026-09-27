#pragma once
#include "CameraScript.h"
#include "CameraEvents.h"
#include <memory>
#include <unordered_map>

namespace CinematicSystem {

class Runtime {
public:
    static Runtime& GetSingleton();

    void Initialize();
    void Shutdown();

    bool Start(std::string filename, bool hideInterface = false);
    void Stop(bool cancelled = true);
    bool IsRunning() const;

    bool StartRecording();
    bool StopRecording();
    bool IsRecording() const;
    bool SaveRecordingPoint(bool absolute);

    float PosX() const;
    float PosY() const;
    float PosZ() const;
    float RotX() const;
    float RotZ() const;
    float FOV() const;
    float TimeMultiplier() const;

    void OnCameraUpdate(RE::TESCamera* camera);
    void EnumerateScripts();
    std::vector<std::string> GetFilenames() const;

private:
    Runtime() = default;
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    struct SavedState {
        bool valid{false};
        bool wasFreeCamera{false};
        bool freeCameraInputHandlingEnabled{true};
        bool wasFreeCameraHandlerRegistered{false};
        bool wasPlayerInputBlocked{false};
        bool wasInterfaceVisible{true};
        bool hideInterface{false};
        RE::NiPoint3 position{};
        float pitch{0.0f};
        float yaw{0.0f};
        float worldFOV{0.0f};
        float firstPersonFOV{0.0f};
        float timeScale{0.0f};
    };

    void SaveState();
    void RestoreState();
    RE::FreeCameraState* GetFreeState() const;
    void Apply(float elapsed);
    void ApplyAction(std::size_t actionIndex, const CameraAction& action, float elapsed);
    void CaptureBaseline(std::size_t actionIndex);
    void ResetToSaved();
    static float Lerp(float a, float b, float t);
    static RE::NiPoint3 CatmullRom(const RE::NiPoint3& p0, const RE::NiPoint3& p1, const RE::NiPoint3& p2, const RE::NiPoint3& p3, float t);

    mutable std::mutex mutex_;
    std::vector<std::string> filenames_;
    std::shared_ptr<const CameraScript> script_;
    std::size_t nextActionIndex_{0};
    std::vector<std::size_t> activeActionIndices_;
    SavedState saved_;
    std::chrono::steady_clock::time_point startTime_{};
    bool running_{false};
    std::atomic_bool playbackUpdateLogged_{false};
    struct ActionBaseline {
        bool captured{false};
        RE::NiPoint3 position{};
        float pitch{0.0f};
        float yaw{0.0f};
        float worldFOV{0.0f};
        float timeScale{0.0f};
    };

    std::unordered_map<std::size_t, ActionBaseline> baselines_;
    bool recording_{false};
    std::chrono::steady_clock::time_point recordingStart_{};
    std::vector<std::string> recordingLines_;
    struct RecordingPoint {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float pitch{0.0f};
        float yaw{0.0f};
        float fov{0.0f};
        float timeMultiplier{0.0f};
    };
    std::optional<RecordingPoint> previousRecordingPoint_;
};

}  // namespace CinematicSystem
