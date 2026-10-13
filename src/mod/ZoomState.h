#pragma once

#include <algorithm>
#include <atomic>

namespace zoom {

// Shared zoom state: written by the key handlers, read by the FOV hook.
// All atomics; safe across the game/render threads that touch them.
class ZoomState {
public:
    static ZoomState& getInstance() {
        static ZoomState instance;
        return instance;
    }

    void setZoomed(bool zoomed) { mZoomed.store(zoomed, std::memory_order_release); }

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

    // Eased divisor for this frame: drifts toward the target (or 1.0 when
    // released) so zoom in/out glides instead of snapping. The factor is
    // intentionally gentle; getFov runs every rendered frame.
    [[nodiscard]] float currentDivisor() {
        const float goal = mZoomed.load(std::memory_order_acquire) ? mTargetDivisor.load(std::memory_order_acquire)
                                                                   : 1.0F;
        float current = mCurrentDivisor.load(std::memory_order_acquire);
        current += (goal - current) * kEasePerFrame;
        if (current < 1.0F) {
            current = 1.0F;
        }
        mCurrentDivisor.store(current, std::memory_order_release);
        return current;
    }

    static constexpr float kDefaultDivisor = 1.5F;
    static constexpr float kMinDivisor     = 1.2F;
    static constexpr float kMaxDivisor     = 30.0F;

private:
    // Per-wheel-tick multiplier (fine control) and per-frame ease toward
    // the target (buttery transitions, including the release zoom-out).
    static constexpr float kScrollFactor = 1.12F;
    static constexpr float kEasePerFrame = 0.18F;

private:
    ZoomState() = default;
    std::atomic<bool>  mZoomed{false};
    std::atomic<float> mTargetDivisor{kDefaultDivisor};
    std::atomic<float> mCurrentDivisor{1.0F};
};

} // namespace zoom
