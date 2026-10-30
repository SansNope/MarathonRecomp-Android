# Test status

[Русская версия](STATUS.ru.md)

> [!IMPORTANT]
> **We do not promise that the port works on every device.** Everything below was checked by the developer and testers on a few phones. Other phones, GPUs and Android versions may not start the game, may show graphics problems or may run slower.

## Tested devices

### POCO F5 — Snapdragon 7+ Gen 2 (SM7475), Adreno 725, Android 17

Settings: driver "A725 Performance (experimental)" (Sysmem), render scale 60 %, VSync, 60 FPS.

- Logos, title screen, menus.
- HUB (Soleanna) with events, a boss fight, Wave Ocean completed (results screen, back to the town), Dusty Desert.
- Sound and music, on-screen touch controls, saving and loading.
- **60 FPS** in the HUB and in levels (Perfetto traces and the built-in `[perf]` log; 59–60 FPS on average outside scene loads).
- Short FPS drops while scenes load (event, boss, level transitions).
- **v0.2.0** (development builds and the final APK): 60 FPS in the HUB and in levels at 60 % render scale, GPU ~13 ms per frame; the optimized Vulkan render passes show no artifacts. With *Keep game threads on fast cores* turned off, the same level ran at 47–55 FPS — keep it on.

### Pixel 7 Pro — Google Tensor G2 (GS201), Mali-G710, Android 17 (tester)

Settings: driver Auto (system Vulkan driver), FPS cap 60, render scale 25 % and 50 %, recommended switches.

- Since v0.1.6: starts and plays (Wave Ocean, about 16 minutes without errors in the first test); menus, HUB, level loads, going to the background and back.
- **Measured with the `[perf]` log on v0.2.0 development builds:** **45–55 FPS** in levels at 25 % render scale, **41–57 FPS** at 50 %, 60 FPS in menus. v0.1.6 ran at 24–31 FPS at 25 %.
- In levels the frame takes 18–24 ms: ~10–12 ms of game-thread work plus ~5–6 ms waiting for the next swap-chain image; the GPU needs 12–17 ms, about 10 ms of which does not depend on the render scale (shadow maps, a chain of tiny bloom passes, full-screen passes).
- No graphics artifacts and no crashes with the optimized render passes; texture cache: no conversions on repeated loads.
- The system Vulkan driver is used, textures are converted to ETC2 (and cached), and `ClipDistance` is removed from the shaders (no `shaderClipDistance`).

### realme RMX5090 — Snapdragon 8 Elite (SM8750), Adreno 830, Android 16

Settings: driver Auto (selects the Adreno 8xx build), render scale 125 %, VSync, 60 FPS.

- HUB with events and a level, no errors in the log.
- Render scale above 100 % (scene 3474×1579 on a 2780×1264 screen).
- Going to the background and back (Vulkan surface recreated).
- Performance described by the tester as excellent; not measured.
- v0.2.x: the graphics artifacts seen with v0.1.6 no longer appear. Their cause in v0.1.6 was not determined.

### Mali-G57 (testers)

- **Helio G99, Mali-G57 MC2 — Samsung Galaxy Tab A9 (Android 15) and an OPPO phone (Android 16), v0.1.6:** starts and plays; the graphics artifacts of the earlier test builds are gone. With render scale 50 %: 27–49 FPS in levels at a 120 FPS cap, limited by the GPU (20–24 ms per frame). At a 30 FPS cap the game itself slowed down (0.3× speed) — caps below 60 are no longer offered since v0.2.0.
- **Helio G99, Mali-G57 MC2 — TECNO KJ7 (Android 14, old Vulkan 1.1 driver), v0.2.0/0.2.1 (tester):** plays; GPU-bound. Render scale 25 %: 35–56 FPS in levels with 1024 shadows, 43–67 FPS (median 54) with *Shadow Resolution* 512. One render-thread crash while a level loaded on v0.2.0 (fixed defensively in v0.2.1). **Loading screens and videos flicker** (cause not found yet).
- **Unisoc T606, Mali-G57 MP1 — Motorola moto g04s (Android 14), v0.1.6:** starts and plays; 10–18 FPS in levels at 100 % render scale, limited by the single-core GPU.
- The system Vulkan driver is used, textures are converted to ETC2, `ClipDistance` is removed from the shaders, and the compact descriptor layout (4 descriptor sets) is used.

## What works

- Building the APK from this repository with the scripts in `android/scripts` (Windows).
- Launcher: game status, install from ZIP/folder, storage choice, driver and render mode, touch options, logs, Info window, English/Russian.
- Bundled Turnip drivers on Adreno 725 and Adreno 830; automatic fallback after a failed driver start.
- Landscape only, both landscape sides.
- Crash report in `log.txt`.
- Since v0.2.0: the game and the sound pause in the background, achievement pop-ups appear; the launcher's *Recommended settings* restores the switches and lists the ones that differ.
- v0.1.5 (Mali support ported from Unleashed, Vulkan 1.2 path) on the POCO F5: starts and plays normally.
- The v0.1.6 Mali workarounds (compact descriptor layout, `ClipDistance` removal, ETC2 textures, 2D-array views) on the POCO F5 with all of them forced on by test markers: picture correct, no errors.
- Updating to the signed v0.1.0 release APK: installed over an earlier debug build on a test phone, saves were kept and the game works. The certificate lineage update path is confirmed on a device.

## Known issues

- **Loading hitches** — scene transitions drop the frame rate for a few seconds.
- **Touch controls are not context-aware yet** — no D-pad in menus, no single Skip button in cutscenes.
- **Anti-aliasing option** — disabled in the in-game menu by upstream MarathonRecomp (not implemented); MSAA is off.
- **Bluetooth controllers through SDL HIDAPI** — the app does not request `BLUETOOTH_CONNECT`; controllers handled by Android itself are expected to work but were not tested.
- **The first run of each scene** may stutter while Vulkan pipelines are created; since v0.2.0 they are kept in a cache on disk for later launches.
- **Helio G99 / Mali-G57: loading screens and videos flicker** (since v0.2.0, also at a 60 FPS cap); not found yet.
- **Adreno 610 (Snapdragon 680)** — broken 2D title/menu graphics reported with the default driver ([#2](https://github.com/SansNope/MarathonRecomp-Android/issues/2)); not investigated yet.
- **Performance on Mali** — the Pixel 7 Pro (Mali-G710) reaches 41–57 FPS in levels but not 60; Mali-G57 devices are much slower. Turn on *Performance log* in the launcher and attach `log.txt`.
- **Frame-rate caps below 60** are not offered on Android since v0.2.0: the game slowed down at 30 FPS (upstream's frame-rate fixes only cover caps above 60).
- **On Mali, user clip planes are disabled** (`ClipDistance` is not supported there); no visible difference has been noticed yet.
- **First loads on Mali are slower** — BC textures are converted to ETC2 on the CPU; since v0.2.0 later loads read them from the texture cache (up to 1 GB of internal storage).
- The debug APK is signed with each builder's own key (see [BUILDING.md](BUILDING.md#signing)).

## Not tested

- Other Adreno generations (6xx, 7xx other than 725, 8xx other than 830).
- Mali GPUs other than G57 MC2/MP1 and G710 (see above). Bifrost GPUs (G31, G51, G52, G71, G72, G76) are not supported.
- Xclipse (Samsung) and PowerVR GPUs (the system Vulkan driver is used there; the same texture fallback applies).
- Android 10–15.
- DLC, a full playthrough, all characters and stages.
- Physical gamepads.
- Long sessions on battery, heat and throttling.
- Render scale above 125 %.
- A complete clean native build from this public checkout (the v0.1.0 native libraries are byte-identical to the tested development build).

## Reporting a problem

Attach `log.txt` (launcher → Advanced → Open logs) and write the phone model, the driver and render mode you used, and what you did before the problem. Please report Android problems to this repository, not to upstream MarathonRecomp.
