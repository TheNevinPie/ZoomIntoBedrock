#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>

namespace zoom {

// Shared zoom state: written by the key handlers, read by the FOV hook.
// All atomics; safe across the game/render threads that touch them.
class ZoomState {
public:
    static ZoomState& getInstance() {
        static ZoomState instance;
        return instance;
    }

    void setZoomed(bool zoomed) {
        mZoomed.store(zoomed, std::memory_order_release);
        if (!zoomed) {
            // Releasing the key always resets the scroll level, so the
            // next zoom starts at the default instead of resuming deep.
            mTargetDivisor.store(kDefaultDivisor, std::memory_order_release);
            mRemainderX.store(0.0F, std::memory_order_release);
            mRemainderY.store(0.0F, std::memory_order_release);
        }
    }

    [[nodiscard]] bool isZoomed() const { return mZoomed.load(std::memory_order_acquire); }

    // Scroll steps while zoomed: +1 zooms in, -1 zooms out.
    // Each step multiplies the target divisor by a small factor so a
    // single wheel detent nudges rather than jumps; the per-frame easing
    // below turns the steps into a smooth glide. Clamped.
    void addZoomSteps(int steps) {
        if (steps == 0) {
            return;
        }
        float target = mTargetDivisor.load(std::memory_order_acquire);
        target *= (steps > 0) ? kScrollFactor : (1.0F / kScrollFactor);
        target = std::clamp(target, kMinDivisor, kMaxDivisor);
        mTargetDivisor.store(target, std::memory_order_release);
    }

    // Eased divisor for this frame: glides toward the target (or 1.0
    // when released) so zoom in/out is a slow smooth travel instead of a
    // snap. Snaps when close so the FOV lands exactly on native/plain.
    [[nodiscard]] float currentDivisor() {
        const float goal = mZoomed.load(std::memory_order_acquire) ? mTargetDivisor.load(std::memory_order_acquire)
                                                                   : 1.0F;
        float current = mCurrentDivisor.load(std::memory_order_acquire);
        if (std::fabs(goal - current) < kSnapThreshold) {
            current = goal;
        } else {
            current += (goal - current) * kEasePerFrame;
        }
        if (current < 1.0F) {
            current = 1.0F;
        }
        mCurrentDivisor.store(current, std::memory_order_release);
        return current;
    }

    // Scales a look-movement delta while zoomed: the deeper the zoom,
    // the heavier the smoothing (sensitivity divides by the zoom level,
    // so high magnification never feels twitchy). Sub-pixel fractions
    // accumulate across events so fine aim never freezes at high zoom.
    // Call only while zoomed; unzoomed play passes through untouched.
    void scaleLookDelta(short dx, short dy, short& outX, short& outY) {
        const float factor = std::max(1.0F, mCurrentDivisor.load(std::memory_order_acquire));
        float remX = mRemainderX.load(std::memory_order_acquire) + static_cast<float>(dx) / factor;
        float remY = mRemainderY.load(std::memory_order_acquire) + static_cast<float>(dy) / factor;
        const auto ox = static_cast<short>(remX); // truncates toward zero
        const auto oy = static_cast<short>(remY);
        mRemainderX.store(remX - static_cast<float>(ox), std::memory_order_release);
        mRemainderY.store(remY - static_cast<float>(oy), std::memory_order_release);
        outX = ox;
        outY = oy;
    }

    static constexpr float kDefaultDivisor = 1.5F;
    static constexpr float kMinDivisor     = 1.2F;
    static constexpr float kMaxDivisor     = 30.0F;

private:
    // Per-wheel-tick multiplier (fine control), slow per-frame glide
    // toward the target, and a snap threshold so motion always settles.
    static constexpr float kScrollFactor  = 1.12F;
    static constexpr float kEasePerFrame  = 0.06F;
    static constexpr float kSnapThreshold = 0.005F;

private:
    ZoomState() = default;
    std::atomic<bool>  mZoomed{false};
    std::atomic<float> mTargetDivisor{kDefaultDivisor};
    std::atomic<float> mCurrentDivisor{1.0F};
    std::atomic<float> mRemainderX{0.0F};
    std::atomic<float> mRemainderY{0.0F};
};

} // namespace zoom
