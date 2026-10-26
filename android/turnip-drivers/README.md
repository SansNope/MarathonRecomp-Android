# Bundled Turnip driver

The APK contains four Turnip binaries, one per selectable driver profile.

The 0.5.3 **Adreno 8xx Experimental** profile (the A725 binary with
`TU_DEBUG=sysmem,flushall`) is superseded by the **Adreno 8xx** profile below; the
launcher maps a saved `ExperimentalA8xx` selection to it.

## Adreno 8xx (Snapdragon 8 Elite)

`vulkan.mesa26_3_gralloc_a8xx.so` is a source-built Mesa main
`6b9cd5ab9cc3bfd0c79d9500a07a5e74351c72d1` (26.3.0-devel) Turnip with
`turnip-driver-ci/patches-unleashed/0010-gralloc-qcom-ubwc-detect.patch`
(`VARIANT=gralloc`). The patch ports whitebelyash/mesa-tu8 `23ec0a5070a`: the newer
Qualcomm gralloc no longer writes the `gmsm` magic, so upstream imports UBWC swapchain
buffers as linear, and the whole image is garbled.

The launcher exposes it as **Adreno 8xx** and forces `TU_DEBUG=sysmem,nobin`. Auto picks
it on SM8750 (Snapdragon 8 Elite, Adreno 830) unless an imported driver is selected.
Measured on Adreno 830 at 100% resolution (Apotos, Average GPU Frame): sysmem 11.95 ms,
GMEM 14.21 ms, stock driver 26.12 ms. All three render cleanly.

## A725 Performance (experimental)

`vulkan.wb26_2_rp_pair_ccu_color_a725.so` is the experimental
whitebelyash/gen8 `7fdde2f8f8` Mesa 26.2 build supplied as
`Turnip-NewDriver-wb-26.2-RP-PAIR-CCU-COLOR.zip`. Its driver SHA-256 is
`87f371a26a4678a092efa0d209be550ccaae0d18c96436aec181d558e53b22b0`.
The render-pass dependency uses the targeted
`CCU_CLEAN_COLOR | WAIT_FOR_ME` mask (`0x202`).

The launcher exposes it as **A725 Performance (experimental)** and forces its
required `TU_DEBUG=sysmem,nobin` preset. It is not selected by Auto and does
not replace the universal bundled driver. This build is currently validated
for the Adreno 725 performance/artifact problem only; it must not become the
default until Adreno 732 support and regression testing are complete.

## Adreno 710 (Vauzi)

`vulkan.vauzi710_v2_7.so` is the unmodified `libvulkan_freedreno.so` from
[Vauzi-17/710 release 2.7](https://github.com/Vauzi-17/710/releases/tag/2.7),
published 2026-07-09. Its release ZIP SHA-256 is
`ebc6cac062f89cb34fc73176ea9092b90dffa71e83e28df9f470d664fad88ee0`.
The extracted driver binary SHA-256 is
`636c7d20cbcaec8ce00f3ee88f4fa0ed8c2312d59dbb9664a7b63e98a387fdb6`.
The package identifies itself as Mesa 26.2.0 / Vulkan 1.4.354 and targets
Adreno 710/720/722. The app exposes it as **Adreno 710 (Vauzi)** and maps
Render Mode **Auto** to `TU_DEBUG=sysmem`, matching the author's stability
recommendation. Explicit GMEM and Sysmem selections remain available.

## Universal Unleashed build

`vulkan.unleashed26_1_wfm_a732.so` — the **"univ" build (2026-07-05)**.

> ⚠️ The filename is **historical** (it used to be the a725/a732/a750 build) and
> is deliberately kept: the app selects the bundled driver by this exact name in
> `driver_name.txt`, and existing installs only re-extract the asset on a *size*
> change — renaming it would strand updated installs on the old driver. The file
> also carries a trailing `UNLEASHED-UNIV-20260705` marker string (harmless to
> ELF loaders) precisely so its size differs from the previous build; `strings`
> it to identify the build.

- **What:** a source-built **Mesa 26.1.4 Turnip** (Adreno open-source Vulkan
  driver, Vulkan 1.4) with three patches baked in:
  - **0001** — unconditional per-draw `TU_CMD_FLAG_WAIT_FOR_ME` (`CP_WAIT_FOR_ME`)
    in `tu6_emit_flushes()`, no `TU_DEBUG` gate. Fixes the a7xx-gen1 "shimmer"
    (a725, and the residual artifacts on a710).
  - **0004** — adds the Adreno **732** chip id to the FD735 device entry.
  - **0009** — adds custom **FD710 / FD720 / FD722** device entries (Adreno
    710/720/722). These do not exist in *any* upstream Mesa; the entries — chip
    ids, gen1 template, and per-GPU magic register tables derived from real
    proprietary-driver command-stream traces — come from
    [Vauzi-17/710](https://github.com/Vauzi-17/710)'s `add_710_720_722.py`
    (a fork of whitebelyash/AdrenoToolsDrivers). Credit to them for that work.
- **Covers:** Adreno **710 / 720 / 722 / 725 / 732 / 750** (a750 additionally
  needs the game's MSAA set to **None** — that is the Android config default;
  a720/a722 entries are untested — no devices yet).
- **soname:** `vulkan.adreno.so` (adrenotools-loadable).

## How it is used
- This exact file is bundled in the APK at
  `android-apk/app/src/main/assets/bundled_driver/` and is extracted to internal
  storage (`files/turnip/`) on first launch, then loaded via **libadrenotools**.
  The asset directory deliberately differs from the extraction directory:
  SDL_RWFromFile resolves relative paths against internal storage *before* the
  APK assets, so an asset under `turnip/` would be permanently shadowed by its
  own extracted copy and APK driver updates would never provision.
- To try a different Turnip build without rebuilding the app, drop a plain `.so`
  into `Android/data/com.sega.sonicunr/files/driver_import/` (from a PC over MTP)
  or `Android/media/com.sega.sonicunr/driver_import/` (browsable by on-device
  file managers) on the device.

## ⚠️ TU_DEBUG must stay `none`
The WFM fix is **compiled in**. On a source build like this, setting
`TU_DEBUG=flushall` enables Mesa's *real* full per-draw cache clean+invalidate —
a huge FPS loss. Leave `tu_debug.txt` at `none` (the default). `flushall` is only
meaningful for the old *binary-patched* stock drivers, where it gates the patched
mask.

## Provenance / rebuilding
Built in CI from a fork of the Turnip build scripts,
**`SansNope/Banners-Turnip`** (branch `unleashed`); the scripts + patches are in
`../turnip-driver-ci/`. Select the variant with the `VARIANT` env var
(**`wfm-univ` produced this file**, patches 0001+0004+0009) and the Mesa ref
with `MESA_REF` (**`mesa-26.1.4`** tag). A `wfm-a710` variant (0001+0008) also
exists for building the same a710/720/722 support on a current Mesa `main`
snapshot instead. Other Adreno 6xx/7xx Turnip builds (e.g. K11MCH1's
AdrenoToolsDrivers, Vauzi-17/710 releases) also work if imported via
`driver_import/`.
