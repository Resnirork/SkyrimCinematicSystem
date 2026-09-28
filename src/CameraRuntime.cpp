#include "CameraRuntime.h"
#include "CameraHook.h"

namespace CinematicSystem {
namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRadPerDeg = kPi / 180.0f;
constexpr float kDegPerRad = 180.0f / kPi;
const std::filesystem::path kScriptDir = std::filesystem::path("Data") / "CameraScripts";
}

Runtime& Runtime::GetSingleton()
{
    static Runtime instance;
    return instance;
}

void Runtime::Initialize()
{
    EnumerateScripts();
    logs::info("runtime initialized with {} script(s).", filenames_.size());
}

void Runtime::Shutdown()
{
    Stop(true);
    logs::info("runtime shut down.");
}

void Runtime::EnumerateScripts()
{
    std::scoped_lock lock(mutex_);
    filenames_.clear();
    std::error_code ec;
    std::filesystem::create_directories(kScriptDir, ec);
    if (ec) {
        logs::error("failed to create script directory '{}': {}", kScriptDir.string(), ec.message());
        return;
    }
    if (!std::filesystem::exists(kScriptDir, ec)) {
        logs::warn("script directory '{}' does not exist.", kScriptDir.string());
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(kScriptDir, ec)) {
        if (ec) {
            logs::error("failed while enumerating scripts: {}", ec.message());
            break;
        }
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".scs") filenames_.push_back(entry.path().filename().string());
    }
    std::sort(filenames_.begin(), filenames_.end());
    logs::debug("found {} script(s).", filenames_.size());
}

std::vector<std::string> Runtime::GetFilenames() const
{
    std::scoped_lock lock(mutex_);
    return filenames_;
}

void Runtime::SaveState()
{
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!camera) return;
    saved_.valid = true;
    saved_.wasFreeCamera = camera->IsInFreeCameraMode();
    if (auto* controls = RE::PlayerControls::GetSingleton()) {
        saved_.wasPlayerInputBlocked = controls->blockPlayerInput;
    }
    if (auto* state = GetFreeState()) {
        saved_.freeCameraInputHandlingEnabled = state->IsInputEventHandlingEnabled();
        if (auto* controls = RE::PlayerControls::GetSingleton()) {
            saved_.wasFreeCameraHandlerRegistered = std::find(controls->handlers.begin(), controls->handlers.end(),
                                                              static_cast<RE::PlayerInputHandler*>(state)) != controls->handlers.end();
        }
    }
    saved_.worldFOV = camera->GetRuntimeData2().worldFOV;
    saved_.firstPersonFOV = camera->GetRuntimeData2().firstPersonFOV;
    if (auto* state = GetFreeState()) {
        saved_.position = state->translation;
        saved_.pitch = state->rotation.x;
        saved_.yaw = state->rotation.y;
    } else {
        saved_.position = camera->GetRuntimeData2().pos;
        saved_.yaw = camera->GetRuntimeData2().yaw;
    }
    if (auto* cal = RE::Calendar::GetSingleton()) saved_.timeScale = cal->GetTimescale();
}

RE::FreeCameraState* Runtime::GetFreeState() const
{
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!camera || !camera->currentState) return nullptr;
    if (camera->currentState->id != RE::CameraState::kFree) return nullptr;
    return static_cast<RE::FreeCameraState*>(camera->currentState.get());
}

bool Runtime::Start(std::string filename, bool hideInterface)
{
    Stop(true);
    EnumerateScripts();

    std::filesystem::path path = kScriptDir / filename;
    std::string error;
    auto parsed = Parser::Parse(path, error);
    if (!parsed) {
        return false;
    }

    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!camera) {
        logs::error("PlayerCamera unavailable; cannot start '{}'.", filename);
        return false;
    }
    SaveState();
    if (auto* ui = RE::UI::GetSingleton()) {
        saved_.wasInterfaceVisible = ui->IsShowingMenus();
        saved_.hideInterface = hideInterface;
        if (saved_.hideInterface) ui->ShowMenus(false);
    }

    if (!camera->IsInFreeCameraMode()) camera->ToggleFreeCameraMode(false);
    auto* freeState = GetFreeState();
    if (!freeState) {
        logs::error("failed to enter FreeCameraState");
        RestoreState();
        return false;
    }
    if (!Hook::Install(freeState)) {
        RestoreState();
        return false;
    }
    freeState->SetInputEventHandlingEnabled(false);
    if (auto* controls = RE::PlayerControls::GetSingleton()) {
        controls->UnregisterHandler(freeState);
        controls->blockPlayerInput = true;
    }
    const auto actionCount = parsed->actions.size();
    const auto scriptDuration = parsed->duration;

    {
        std::scoped_lock lock(mutex_);
        script_ = std::make_shared<CameraScript>(std::move(*parsed));
        nextActionIndex_ = 0;
        activeActionIndices_.clear();
        baselines_.clear();
        running_ = true;
        playbackUpdateLogged_.store(false);
        startTime_ = std::chrono::steady_clock::now();
    }
    logs::info("started {} ({} action(s), duration {:.3f}s).", filename, actionCount, scriptDuration);
    return true;
}

void Runtime::Stop(bool cancelled)
{
    std::string name;
    bool shouldFinish = false;
    {
        std::scoped_lock lock(mutex_);
        if (!running_) return;
        shouldFinish = true;
        name = script_ ? script_->filename : std::string{};
        running_ = false;
        activeActionIndices_.clear();
        nextActionIndex_ = 0;
        baselines_.clear();
    }
    if (shouldFinish) {
        RestoreState();
        Events::ScriptFinished(name, cancelled);
        logs::info("stopped {}{}.", name, cancelled ? " (cancelled)" : "");
        std::scoped_lock lock(mutex_);
        script_.reset();
    }
}

bool Runtime::IsRunning() const
{
    std::scoped_lock lock(mutex_);
    return running_;
}

float Runtime::Lerp(float a, float b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

RE::NiPoint3 Runtime::CatmullRom(const RE::NiPoint3& p0, const RE::NiPoint3& p1,
                                 const RE::NiPoint3& p2, const RE::NiPoint3& p3, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    return {
        0.5f * (2.0f * p1.x + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
        0.5f * (2.0f * p1.y + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3),
        0.5f * (2.0f * p1.z + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3)
    };
}

void Runtime::CaptureBaseline(std::size_t actionIndex)
{
    std::scoped_lock lock(mutex_);
    if (baselines_.contains(actionIndex)) return;

    ActionBaseline baseline;
    if (auto* state = GetFreeState()) {
        baseline.position = state->translation;
        baseline.pitch = state->rotation.x;
        baseline.yaw = state->rotation.y;
    }
    if (auto* camera = RE::PlayerCamera::GetSingleton()) baseline.worldFOV = camera->GetRuntimeData2().worldFOV;
    if (auto* calendar = RE::Calendar::GetSingleton()) baseline.timeScale = calendar->GetTimescale();
    baseline.captured = true;
    baselines_.emplace(actionIndex, baseline);
}

void Runtime::ResetToSaved()
{
    auto* state = GetFreeState();
    if (!state || !saved_.valid) return;
    state->translation = saved_.position;
    state->rotation.x = saved_.pitch;
    state->rotation.y = saved_.yaw;
    auto* camera = RE::PlayerCamera::GetSingleton();
    camera->GetRuntimeData2().worldFOV = saved_.worldFOV;
    camera->GetRuntimeData2().firstPersonFOV = saved_.firstPersonFOV;
}

void Runtime::ApplyAction(std::size_t actionIndex, const CameraAction& a, float elapsed)
{
    auto* state = GetFreeState();
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!state || !camera) return;

    if (std::isnan(a.scalar)) {
        if (elapsed >= a.start) ResetToSaved();
        return;
    }
    if (elapsed < a.start) return;

    CaptureBaseline(actionIndex);

    ActionBaseline baseline;
    {
        std::scoped_lock lock(mutex_);
        baseline = baselines_.at(actionIndex);
    }

    const float t = a.duration <= 0.0f ? 1.0f : std::clamp((elapsed - a.start) / a.duration, 0.0f, 1.0f);
    logs::trace("applying action {} (type: {}, mode: {}, t: {})", actionIndex,
                static_cast<int>(a.type), static_cast<int>(a.mode), t);

    if (a.type == ActionType::Move) {
        if (!a.points.empty()) {
            std::vector<RE::NiPoint3> points;
            points.reserve(a.points.size() + 1);
            points.push_back(baseline.position);
            for (const auto& point : a.points) {
                points.push_back(a.mode == ValueMode::Relative
                                     ? RE::NiPoint3{baseline.position.x + point.x, baseline.position.y + point.y, baseline.position.z + point.z}
                                     : point);
            }

            const float segmentPosition = t * static_cast<float>(points.size() - 1);
            const auto segment = static_cast<std::size_t>(segmentPosition) < points.size() - 2
                                     ? static_cast<std::size_t>(segmentPosition)
                                     : points.size() - 2;
            const float localT = segmentPosition - static_cast<float>(segment);
            const auto& p1 = points[segment];
            const auto& p2 = points[segment + 1];
            const auto& p0 = points[segment == 0 ? 0 : segment - 1];
            const auto& p3 = points[segment + 2 < points.size() ? segment + 2 : points.size() - 1];
            state->translation = CatmullRom(p0, p1, p2, p3, localT);
        } else {
            const RE::NiPoint3 target = a.mode == ValueMode::Absolute
                                            ? a.vector
                                            : RE::NiPoint3{baseline.position.x + a.vector.x, baseline.position.y + a.vector.y, baseline.position.z + a.vector.z};
            state->translation = {
                Lerp(baseline.position.x, target.x, t),
                Lerp(baseline.position.y, target.y, t),
                Lerp(baseline.position.z, target.z, t)
            };
        }
    } else if (a.type == ActionType::Rotate) {
        const float pitchDelta = -a.vector.x * kRadPerDeg;
        const float targetPitch = a.mode == ValueMode::Absolute ? pitchDelta : baseline.pitch + pitchDelta;
        const float targetYaw = a.mode == ValueMode::Absolute ? a.vector.y * kRadPerDeg : baseline.yaw + a.vector.y * kRadPerDeg;
        state->rotation.x = Lerp(baseline.pitch, targetPitch, t);
        state->rotation.y = Lerp(baseline.yaw, targetYaw, t);
    } else if (a.type == ActionType::FOV) {
        const float target = a.mode == ValueMode::Absolute ? a.scalar : baseline.worldFOV + a.scalar;
            auto& runtimeData = camera->GetRuntimeData2();
            runtimeData.worldFOV = Lerp(baseline.worldFOV, target, t);
    } else if (a.type == ActionType::Time) {
        auto* cal = RE::Calendar::GetSingleton();
        if (cal && cal->timeScale) {
            const float target = a.mode == ValueMode::Absolute ? a.scalar : baseline.timeScale + a.scalar;
            cal->timeScale->value = Lerp(baseline.timeScale, target, t);
        }
    }
}

void Runtime::Apply(float elapsed)
{
    std::shared_ptr<const CameraScript> local;
    std::vector<std::size_t> startedActions;
    std::vector<std::size_t> activeActions;
    std::vector<std::pair<std::size_t, std::size_t>> supersededActions;
    {
        std::scoped_lock lock(mutex_);
        local = script_;
        if (!local) return;
        while (nextActionIndex_ < local->actions.size() && local->actions[nextActionIndex_].start <= elapsed) {
            const auto index = nextActionIndex_++;
            const auto& newAction = local->actions[index];
            std::erase_if(activeActionIndices_, [&](std::size_t activeIndex) {
                const auto& activeAction = local->actions[activeIndex];
                const bool overlapsSameType = activeAction.type == newAction.type &&
                                               activeAction.start + activeAction.duration > newAction.start;
                if (overlapsSameType) supersededActions.emplace_back(activeIndex, index);
                return overlapsSameType;
            });
            activeActionIndices_.push_back(index);
            startedActions.push_back(index);
        }
        activeActions = activeActionIndices_;
    }

    for (const auto& [oldIndex, newIndex] : supersededActions) {
        logs::debug("action {} superseded by action {} of the same type.", oldIndex, newIndex);
    }

    for (const auto index : startedActions) {
        const auto& action = local->actions[index];
        logs::debug("action {} started at {:.3f}s (duration {:.3f}s).", index, action.start, action.duration);
    }

    std::vector<std::size_t> completedActions;
    for (const auto index : activeActions) {
        const auto& action = local->actions[index];
        ApplyAction(index, action, elapsed);
        if (std::isnan(action.scalar) || elapsed >= action.start + action.duration) {
            completedActions.push_back(index);
            logs::debug("action {} completed at {:.3f}s.", index, elapsed);
        }
    }

    if (!completedActions.empty()) {
        std::scoped_lock lock(mutex_);
        if (script_ == local) {
            std::erase_if(activeActionIndices_, [&completedActions](std::size_t index) {
                return std::binary_search(completedActions.begin(), completedActions.end(), index);
            });
        }
    }
}

void Runtime::OnCameraUpdate(RE::TESCamera* a_camera)
{
    auto* playerCamera = RE::PlayerCamera::GetSingleton();
    if (!playerCamera || a_camera != playerCamera || !IsRunning()) return;

    const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime_).count();
    if (!playbackUpdateLogged_.exchange(true)) {
        logs::debug("playback update reached at {:.3f}s; free camera state {}.",
                    elapsed, GetFreeState() ? "available" : "missing");
    }
    if (!GetFreeState()) {
        logs::error("FreeCameraState lost during playback at {:.3f}s; stopping script.", elapsed);
        Stop(true);
        return;
    }
    Apply(elapsed);

    std::shared_ptr<const CameraScript> local;
    {
        std::scoped_lock lock(mutex_);
        local = script_;
    }
    if (local && elapsed >= local->duration) Stop(false);
}

void Runtime::RestoreState()
{
    auto* camera = RE::PlayerCamera::GetSingleton();
    if (!camera || !saved_.valid) return;

    if (!camera->IsInFreeCameraMode()) camera->ToggleFreeCameraMode(false);
    ResetToSaved();
    if (auto* cal = RE::Calendar::GetSingleton(); cal && cal->timeScale) cal->timeScale->value = saved_.timeScale;

    if (auto* state = GetFreeState()) {
        state->SetInputEventHandlingEnabled(saved_.freeCameraInputHandlingEnabled);
        if (saved_.wasFreeCamera && saved_.wasFreeCameraHandlerRegistered) {
            if (auto* controls = RE::PlayerControls::GetSingleton()) {
                controls->RegisterHandler(state, true);
            }
        }
        Hook::Uninstall(state);
    }
    if (auto* controls = RE::PlayerControls::GetSingleton()) {
        controls->blockPlayerInput = saved_.wasPlayerInputBlocked;
    }
    if (saved_.hideInterface) {
        if (auto* ui = RE::UI::GetSingleton()) {
            ui->ShowMenus(saved_.wasInterfaceVisible);
        }
    }

    if (!saved_.wasFreeCamera && camera->IsInFreeCameraMode()) camera->ToggleFreeCameraMode(false);
    saved_.valid = false;
}

bool Runtime::StartRecording()
{
    if (recording_) {
        logs::warn("recording is already active.");
        return false;
    }
    recording_ = true;
    recordingStart_ = std::chrono::steady_clock::now();
    recordingLines_.clear();
    previousRecordingPoint_.reset();
    logs::info("recording started.");
    return true;
}

bool Runtime::StopRecording()
{
    if (!recording_) {
        logs::warn("cannot stop recording because no recording is active.");
        return false;
    }
    recording_ = false;
    previousRecordingPoint_.reset();
    if (recordingLines_.empty()) {
        logs::info("recording stopped without captured points.");
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(kScriptDir, ec);
    std::ofstream out(kScriptDir / "recording.scs");
    if (!out) {
        logs::error("failed to write recording '{}'.", (kScriptDir / "recording.scs").string());
        return false;
    }
    out << "# CinematicSystem recording\n";
    for (const auto& line : recordingLines_) out << line << '\n';
    logs::info("recording saved to '{}' ({} line(s)).", (kScriptDir / "recording.scs").string(), recordingLines_.size());
    return true;
}

bool Runtime::IsRecording() const { return recording_; }

bool Runtime::SaveRecordingPoint(bool absolute)
{
    if (!recording_) {
        logs::warn("cannot save recording point because no recording is active.");
        return false;
    }
    const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - recordingStart_).count();
    const float x = PosX(), y = PosY(), z = PosZ(), rx = RotX(), rz = RotZ(), fov = FOV(), tm = TimeMultiplier();
    const RecordingPoint currentPoint{x, y, z, rx, rz, fov, tm};
    if (absolute) {
        recordingLines_.push_back(std::format("mov,{:.3f},0,{{{:.3f},{:.3f},{:.3f}}}", t, x, y, z));
        recordingLines_.push_back(std::format("rot,{:.3f},0,{{{:.3f},{:.3f}}}", t, rx, rz));
        recordingLines_.push_back(std::format("fov,{:.3f},0,{{{:.3f}}}", t, fov));
    } else {
        const auto& previous = previousRecordingPoint_.value_or(currentPoint);
        recordingLines_.push_back(std::format("mov,{:.3f},0,[{:.3f},{:.3f},{:.3f}]", t,
                                               x - previous.x, y - previous.y, z - previous.z));
        recordingLines_.push_back(std::format("rot,{:.3f},0,[{:.3f},{:.3f}]", t,
                                               rx - previous.pitch, rz - previous.yaw));
        recordingLines_.push_back(std::format("fov,{:.3f},0,[{:.3f}]", t, fov - previous.fov));
    }
    previousRecordingPoint_ = currentPoint;
    return true;
}

float Runtime::PosX() const { if (auto* s = GetFreeState()) return s->translation.x; return 0.0f; }
float Runtime::PosY() const { if (auto* s = GetFreeState()) return s->translation.y; return 0.0f; }
float Runtime::PosZ() const { if (auto* s = GetFreeState()) return s->translation.z; return 0.0f; }
float Runtime::RotX() const { if (auto* s = GetFreeState()) return -s->rotation.x * kDegPerRad; return 0.0f; }
float Runtime::RotZ() const { if (auto* s = GetFreeState()) return s->rotation.y * kDegPerRad; return 0.0f; }
float Runtime::FOV() const { if (auto* c = RE::PlayerCamera::GetSingleton()) return c->GetRuntimeData2().worldFOV; return 0.0f; }
float Runtime::TimeMultiplier() const { if (auto* c = RE::Calendar::GetSingleton()) return c->GetTimescale(); return 0.0f; }

}  // namespace CinematicSystem
