// Zoom hooks (client only).
//
// Evidence for every engine symbol (all on the 26.51 line):
// - LL_AUTO_TYPE_INSTANCE_HOOK pattern: pocket-on-pc ReachHook.cpp
//   (GameMode::$getPickRange) — same mod family, same toolchain.
// - LevelRendererPlayer::getFov(float, bool) override: Playback
//   ReplayCameraFovHook (v0.3.3-mc26.50) overrides the return value the
//   same way; the LevelRendererPlayer path reports degrees.
// - MouseDevice::feed + MouseAction::ActionWheel/DataUp/DataDown:
//   LeviLamina headers (mc/deps/input/MouseDevice.h, MouseAction.h).
//   While zoomed, wheel events adjust the zoom level and are swallowed
//   (hotbar does not scroll); everything else passes through untouched.
#include "mod/ZoomState.h"

#include "ll/api/memory/Hook.h"
#include "mc/client/renderer/game/LevelRendererPlayer.h"
#include "mc/deps/input/MouseAction.h"
#include "mc/deps/input/MouseDevice.h"

#include <cmath>

namespace zoom {
namespace {

constexpr float kMinSaneFov = 1.0F;
constexpr float kMaxSaneFov = 179.0F;
// Wheel direction convention (MouseAction::DataUp = scroll up = zoom in)
// comes from the header enum names. If in-game testing shows inverted
// behavior on some mouse, flip this one constant instead of touching logic.
constexpr bool kInvertScrollWheel = false;

LL_AUTO_TYPE_INSTANCE_HOOK(
    ZoomGetFovHook,
    ll::memory::HookPriority::Normal,
    LevelRendererPlayer,
    &LevelRendererPlayer::getFov,
    float,
    float amount,
    bool  enableVariableFov
) {
    const float native = origin(amount, enableVariableFov);
    if (!std::isfinite(native) || native <= kMinSaneFov || native >= kMaxSaneFov) {
        return native;
    }
    const float divisor = ZoomState::getInstance().currentDivisor();
    if (divisor <= 1.0F) {
        return native;
    }
    const float zoomed = native / divisor;
    return (zoomed >= kMinSaneFov && zoomed <= kMaxSaneFov) ? zoomed : native;
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    ZoomWheelHook,
    ll::memory::HookPriority::Normal,
    MouseDevice,
    &MouseDevice::feed,
    void,
    char        actionButtonId,
    schar       buttonData,
    short       x,
    short       y,
    short       dx,
    short       dy,
    bool        forceMotionlessPointer
) {
    if (actionButtonId == ::MouseAction::ActionWheel && ZoomState::getInstance().isZoomed()) {
        // Wheel up zooms in, wheel down zooms out. The event is swallowed
        // so the hotbar does not scroll while adjusting zoom; every other
        // input (including the wheel when NOT zoomed) passes through.
        const bool down = (buttonData == ::MouseAction::DataDown) != kInvertScrollWheel;
        ZoomState::getInstance().addZoomSteps(down ? -1 : 1);
        return;
    }
    origin(actionButtonId, buttonData, x, y, dx, dy, forceMotionlessPointer);
}

} // namespace
} // namespace zoom
