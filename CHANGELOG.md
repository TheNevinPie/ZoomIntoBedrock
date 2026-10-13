# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Hold-C zoom (1.5x default, remappable) via `getFov` hook.
- Scroll-while-zoomed level adjustment (1.2x–30x, slow smooth glide).

### Fixed

- Smoother, much slower zoom glide (ease 0.06/frame + settle snap).
- Scroll-out reliability: wheel direction now accepts signed `dy`,
  `DataUp`/`DataDown`, or signed data magnitude, so zoom-out can never
  be misread as zoom-in (previous code only read the data flag and
  could wedge at maximum). `kInvertScrollWheel` escape hatch kept.
- Release resets the scroll level to default (next zoom always 1.5x).
- Zoom-depth look smoothing: sensitivity divides by zoom level with
  fractional accumulation.
