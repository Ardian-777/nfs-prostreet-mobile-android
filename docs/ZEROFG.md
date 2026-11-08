# ZeroFG in Xendroid+

ZeroFG generates the frame halfway between two of the game's frames, so the
screen shows twice the game's frame rate: 30 fps becomes 60, 60 becomes 120. It
is **off by default** and has two modes:

- **Zero**: the quality engine. It rebuilds the scene as moving layers
  (surfaces, the nearest moving object with its own matte, a screen layer for
  HUDs), judged against a dense-flow picture. It leads when the camera moves.
- **ReallyZero**: the economy engine. A dense optical flow with a cleanup pass,
  at a fraction of Zero's cost and close to it wherever the camera holds still.

The engine is ZeroFG's public release **v1.1.0** (Apache-2.0), vendored in
`emulator-core/src/main/cpp/zerofg` (provenance in its `SOURCE.md`). How the
engines work, and how they compare with LSFG 3.1 and win-fg clip by clip, is in
the [ZeroFG README](https://github.com/hy300leosquizz-ctrl/ZeroFG#readme).

## Turning it on and off

- **Settings → Frame generation (ZeroFG):** Off, ReallyZero or Zero.
- **In game, the Frame generation menu:** the same three choices, with the
  menu's usual Keep, Undo and Make global.
- **Recommended:** set the game's **Frame rate limit** to its own rate (30 or
  60). Its frames then arrive evenly, and so do the generated ones (see
  [Notes for heavy games](#notes-for-heavy-games)).

The mode switches while the game runs. The renderer disconnects and drains the
old session, returns the window, and builds the new mode's state; nothing is
destroyed from the Android UI callback. Only one frame-generation engine runs
at a time: choosing another one turns ZeroFG off, and choosing ZeroFG stops the
other. The menu reports the requested mode and the output that is actually
running separately, so it never calls an unfinished switch active.

ZeroFG needs Vulkan 1.3 with synchronization2 and the storage formats, and the
Android image/fence handoff (AHardwareBuffer and sync fds). These are
negotiated when the device supports them, even while ZeroFG is off, so it can be
switched on later without restarting the game. A device that does not qualify
keeps the native presentation; the Plus Vulkan floor does not change.

## How it presents: the ZeroFG Independent Presenter

ZeroFG brings its own presenter instead of generating inside Xenia's:

- **A second Vulkan device (device B)** on the same GPU, beside Xenia's
  (device A). Device A renders the game as before. Each new game frame is
  copied into an AHardwareBuffer image and handed to device B with a sync fd
  (`zerofg_device_handoff`), so device B never waits on device A's queue.
- **Device B owns the window while ZeroFG runs** (main surface authority). It
  generates the middle frames and presents real and generated frames in order;
  Xenia's presenter keeps rendering but gives the surface to it, and takes it
  back on Off or on any failure.
- **GPU priority.** Device B asks for `VK_QUEUE_GLOBAL_PRIORITY_HIGH`
  (VK_KHR/EXT_global_priority). With a custom driver loaded through
  adrenotools, a small hook (`kgsl_priority_hook.c`) also raises its KGSL
  context priority, so its short work never queues behind a busy game.
- **Guest vblank.** The Plus `VblankPacer` stays. While ZeroFG is live, a pull
  may advance one vblank event and repays it with the next; it cannot advance a
  second one before that. With ZeroFG off, the native grid is unchanged.
- **Image controls.** The Plus scaling, CAS, FSR, dither and color filter run
  on real and generated frames alike, with private resources per output job.
  Refresh-rate votes follow whoever owns the window, TV output included.

## Pacing

Generating the middle frame is half the work. The other half, and the one that
took longest, is showing every frame at the right moment without holding the
game back. It is not perfect, nothing is, but it comes close:

- **It follows the game's real clock.** The game's period is measured from the
  issue times of its real frames, not taken as a fixed number. A calendar that
  fell behind comes back to a small cushion, and one that ran ahead drains
  through the rate, without skipping a frame or taking real frames out of order
  (phase recovery, with the rate lock, presentation advance and reanchor
  cutover).
- **Every frame on its refresh.** Each present carries its desired time
  through `VK_GOOGLE_display_timing`, lined up with the panel's vsync (the vsync
  quantizer), and the driver's actual presentation time comes back for the
  measurements. The output keeps up when the system changes the panel's rate,
  such as HyperOS dropping it to 90 Hz.
- **It does not hold the game.** The game waits for ZeroFG only at a bounded
  handoff depth, and its wait for its own GPU is measured apart and never
  counted as ZeroFG's backpressure. With device B's priority, the copy of each
  frame with the GPU 95-99 % busy went from 40-170 ms to 1.3 ms.
- **Guest vblank.** A late game frame may pull one vblank early, repaid by the
  next, instead of losing a whole refresh.

Measured with this presenter on the Adreno 840:

| Game (host) | Before | With the current pacing |
|---|---|---|
| Naruto, PPSSPP, 30 → 60 | uneven 1-3 refresh pairs | an even 2 · 2 |
| Resident Evil 4, ARMSX2 | | the game's frame on screen about 13 ms sooner, 0-3 % of generated frames late |
| Arkham City, XenDroid, 30 fps cap | 38 % of intervals at two refreshes | 79-89 % |
| Gears of War 3, Xendroid+, 30 → 60 | | even pacing, 46-48 ms over Off at the median |
| Halo 3, Xendroid+, 30 fps cap | | output at 60, 16.7 ms intervals at the median and p90 |

The presenter's invariant counters (accepted real dropped, real out of order and
the others) stayed at zero in every window of the Xendroid+ sessions.

## Changes to Xenia's presenter

- `IssueSwap` records the Source present boundary (`RecordSourcePresent`),
  which identifies each game frame for the handoff and the pacing.
- **Paint pipeline fix (native presentation too):** Xenia never recorded the
  format a swapchain paint pipeline was created for, so its format check rebuilt
  the pipeline on every paint. The format is now recorded and the pipeline is
  rebuilt only when the swapchain format changes.
- The window handover and the refresh-rate votes follow the main surface
  authority.
- `frame_stats.h` keeps the Source cadence statistics the pacing reads, with a
  native test.

## SGSR1 Edge Direction

The SGSR1 Edge Direction filter from XenDroid, with an optional dither variant,
in Settings and in the in-game Image menu. It is appended as scaling value 6:
existing values keep their indices and the default filter does not change. The
existing SGSR1 and SGSR1+dither SPIR-V stayed byte-identical; both Edge shaders
pass `spirv-val --target-env vulkan1.0`.

## Tested

- **Phone:** Xiaomi 25102PCBEG (Adreno 840, Android 16), Turnip.
  - Gears of War 3, one fixed scene, Off → ReallyZero → Off → Zero → Off:
    the real frames reach the display 46.5 ms (ReallyZero) and 47.8 ms (Zero)
    later than Off at the median (Off: 26.8 ms), with even pacing at 30 → 60.
    This is IssueSwap to the driver's reported presentation time, not
    controller input to photon.
  - Halo 3 with its frame-rate limit at 30, both modes and switches while
    playing: output at 60, with intervals of 16.7 ms at the median and p90.
  - Both were measured with this port's previous build, whose engine (ZeroFG
    `38dcc6f`) v1.1.0 extends only with the HUD-plane path, which Xendroid+
    does not use.
- **Build and tests:** the debug APK builds with JDK 21 and NDK 29.0.14206865;
  the JVM tests cover the in-game menu, Keep/Undo, the settings catalog and
  schema, and the English and Portuguese strings; the JNI declarations and the
  native vblank-pacer and frame-statistics tests pass.
- **Other GPUs:** the engines are measured on the Adreno 650 and a Radeon Vega
  in the ZeroFG laboratory. Mali and Xclipse were not tested in this host.

## Notes for heavy games

A game that paces itself by vblank and is limited by the GPU (Halo 3 in its
heavier stretches) can deliver frames unevenly: a late frame, then several
quick ones to catch up. ZeroFG then shows some real frames without a generated
one between them. **Setting the game's frame-rate limit to its own rate (30 for
Halo 3)** keeps the frames even and the output at 60.

## Diagnostics

`xe.log` carries a summary about every five seconds while ZeroFG runs:

- `ZeroFGPacing` is the presenter's period against the game's.
- `ZeroFGMainSurface` has the displayed intervals and the presents, real and
  generated.
- `ZeroFGHandoff` has the game's waits: `source_gpu_depth_wait_ms` is the game
  waiting on its own GPU, and `source_bp_waits/ms` is the game waiting on
  ZeroFG.
- `ZeroFGLatencyBatch` has the real frames' IssueSwap-to-display samples, for
  Off and both modes.
