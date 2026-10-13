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
// Wheel direction convention: the header only names DataUp/DataDown, but
// the live event may carry direction as the signed dy motion, as the data
// flag, or as a signed data magnitude. wheelScrolledDown() accepts all
// three (dy first, then flag, then signed data) so one unknown encoding
// cannot wedge the zoom at an endpoint. If in-game testing shows the
// direction is exactly backwards, flip kInvertScrollWheel (one constant).
constexpr bool kInvertScrollWheel = false;

[[nodiscard]] bool wheelScrolledDown(short dy, schar buttonData) {
    bool down;
    if (dy != 0) {
        down = (dy < 0);
    } else if (buttonData == ::MouseAction::DataUp || buttonData == ::MouseAction::DataDown) {
        down = (buttonData == ::MouseAction::DataDown);
    } else {
        down = (buttonData < 0);
    }
    return down != kInvertScrollWheel;
}

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
    if (ZoomState::getInstance().isZoomed()) {
        if (actionButtonId == ::MouseAction::ActionWheel) {
            // Wheel up zooms in, wheel down zooms out. The event is
            // swallowed so the hotbar does not scroll while adjusting
            // zoom. Because zoom-out can never be misread as zoom-in
            // (see wheelScrolledDown), scrolling back always escapes
            // the maximum.
            ZoomState::getInstance().addZoomSteps(wheelScrolledDown(dy, buttonData) ? -1 : 1);
            return;
        }
        if (actionButtonId == ::MouseAction::ActionMove || actionButtonId == ::MouseAction::ActionMoveRelative) {
            // Look smoothing that deepens with zoom: sensitivity divides
            // by the zoom level, so high magnification never feels
            // twitchy. Unzoomed input never reaches this branch.
            short sx = 0, sy = 0;
            ZoomState::getInstance().scaleLookDelta(dx, dy, sx, sy);
            origin(actionButtonId, buttonData, x, y, sx, sy, forceMotionlessPointer);
            return;
        }
    }
    origin(actionButtonId, buttonData, x, y, dx, dy, forceMotionlessPointer);
}

} // namespace
} // namespace zoom
