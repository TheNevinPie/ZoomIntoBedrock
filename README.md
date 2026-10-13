# ZoomIntoBedrock

Hold-to-zoom camera for Minecraft Bedrock (Windows), as a native
LeviLamina **client** mod (C++23).

- **Hold `C`** to zoom (1.5x by default, remappable in Settings > Keyboard).
- **Scroll** while zoomed to zoom further in/out (1.2x–30x). Zoom
  transitions are a slow smooth glide, including the zoom-out on release.
- Scrolling is only redirected while zoomed; otherwise the hotbar scrolls normally.

## Build

```powershell
xmake f -a x64 -m release -p windows -v --target_type=client -y
xmake -v -y
```

Pinned to LeviLamina `26.51.*` (client). After a successful build the
mod is in `bin/`.

## How it works

- Zoom key: `ll::input::KeyRegistry` (`zoom`, default `C` = 67 per
  `mc/deps/input/Keyboard.h`, remappable).
- Zoom itself: `LevelRendererPlayer::getFov` hook returns
  `native / divisor` while zoomed (same override point Playback uses).
- Scroll: `MouseDevice::feed` hook catches `ActionWheel` while zoomed,
  steps the divisor (`DataUp` = in, `DataDown` = out), and swallows the
  event so the hotbar stays put.

## Notes

- Client-side visual only; nothing is sent to the server.
- Mouse sensitivity is unchanged while zoomed (possible follow-up).
- License: CC0-1.0 (see LICENSE; change if you need otherwise).
