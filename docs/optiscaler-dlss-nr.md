# Neural rendering with OptiScaler (experimental)

The `exp/dlss-nr` branch lets the
[OptiScaler-F5-DLSSNR-Multipass](https://github.com/janblade/OptiScaler-F5-DLSSNR-Multipass)
mod (janblade's OptiScaler fork, "DLSS5") apply NVIDIA's DLSS neural rendering
(NR) model to the client. NR redraws the picture's lighting, detail and colour.

Nothing from OptiScaler or NVIDIA is in this repository or in the build. A
player installs the mod next to `Main.exe` by hand and supplies the NR model
file themselves. Without the mod, the client runs exactly as before.

## How it works

The client makes no DLSS, FSR or XeSS call. OptiScaler handles games like that
with its **Optical F5Low** mode:

1. OptiScaler loads as a stand-in for a Windows DLL (`winmm.dll`) and hooks the
   client's Direct3D 12 device and swapchain.
2. Its depth finder watches the client's draws and picks the 3D scene's depth
   buffer.
3. Each frame, it estimates motion from the presented picture (optical flow),
   checked against that depth.
4. It runs NR on the finished picture before it is shown.

Because NR sees the finished frame, it also processes the HUD, windows and
text. Because motion is estimated rather than reported by the engine, fast
effects and moving characters can smear.

## What the branch changes

Only the SDL submodule. It points at the fork
[nitoygo/SDL](https://github.com/nitoygo/SDL), branch `mu/d3d12-native-interop`,
which is SDL 3.4.8 plus three changes to the Direct3D 12 backend:

- **Swapchain size.** SDL created its swapchain as 0x0, meaning "the window's
  size". OptiScaler treats any swapchain under 100x100 as an overlay (Steam,
  Discord) and leaves it unhooked, so it never saw the client's frames. The
  swapchain is now created at the window's size in pixels.
- **Swapchain format.** OptiScaler's finished-picture NR accepts RGBA swapchains
  but rejects SDL's BGRA one on every frame ("This screen colour format is not
  supported."). The SDR swapchain is now `R8G8B8A8_UNORM`. The client's
  screenshot and RmlUi readback code already handle both channel orders.
- **Native handles.** `SDL_GetGPUCommandBufferNativeHandle`,
  `SDL_GetGPUTextureNativeHandle`, and the Direct3D 12 device and command queue
  as GPU device properties. Nothing uses them yet; they exist for an in-engine
  DLSS call (see [Next step](#next-step)).

The client's own source is unchanged.

## Which GPUs can run it

| GPU | What it needs | Status |
|---|---|---|
| NVIDIA RTX 50 | The release's default forwarder and NVIDIA's original 310.8 model | Should work; not tested with the client |
| NVIDIA RTX 20, 30, 40 | The default forwarder and ShortFuse's compatibility 310.8 model | Should work; not tested with the client |
| AMD or Intel, Direct3D 12 | The vendor-neutral forwarder and the 310.9 model | Hooking, motion and depth verified on an RX 5700 XT; NR itself not yet run |
| NVIDIA GTX and older | Not supported by the mod | No |
| Linux, macOS | Not supported (Windows Direct3D 12 only) | No |

NVIDIA cards need driver 616.56 or newer. The vendor-neutral forwarder runs the
model in DirectML. janblade has only tested it on NVIDIA cards, and on GPUs
without AI matrix units (such as RDNA1) it may be too slow to be useful.

## Setup

1. Check out `exp/dlss-nr` and update the submodules. A clone that was on
   `main` must first pick up the new SDL URL:

   ```
   git submodule sync src/ThirdParty/SDL
   git submodule update --init src/ThirdParty/SDL
   ```

2. Build as usual.
3. In the client's `config.ini`, under `[Render]`, set `Backend=d3d12`. Windows
   defaults to Vulkan, and the finished-picture mode needs Direct3D 12.
4. Download the OptiScaler-DLSSNR-F5 release zip (tested with v0.1.32). Extract
   everything except `docs` into the folder with `Main.exe`, then rename
   `OptiScaler.dll` to `winmm.dll`. Running `setup_windows.bat` and picking
   `winmm.dll` does the same.
5. Add the model, `nvngx_dlssnr.dll`, to the same folder, next to `Main.exe`.
   It is not in the zip. Which one depends on the GPU; see
   [NVIDIA GPUs](#nvidia-gpus) or [AMD and Intel GPUs](#amd-and-intel-gpus).
6. In `OptiScaler.ini`, set these (the menu's Optical F5Low page, mode "NR
   only", sets the `[DlssNr]` ones):

   ```ini
   [DlssNr]
   Enabled=true
   NativeDepthFinder=true
   NativeMotion=true
   NativeInput=true
   NativeUpscaler=false
   FinishedPicture=true

   [Spoofing]
   Dxgi=false

   [ProcessFilter]
   TargetProcessName=Main.exe
   ```

   `Dxgi=false` turns off pretending to be an NVIDIA card. The client never
   calls DLSS, so nothing needs it.

7. Start `Main.exe`. **Insert** opens the OptiScaler menu, and **backslash**
   turns NR on and off.

### NVIDIA GPUs

NR runs through NVIDIA's NGX in the driver, so it needs an RTX card and driver
616.56 or newer.

1. Keep the release's own `nvngx.dll_dlssnr.dll` (the NGX forwarder). Don't
   copy anything from the vendor-neutral zip.
2. Add the model for your card. janblade's `INSTALL-DLSSNR.md` (in the release
   zip) is the authority on versions and hashes; at v0.1.32 it says:

   | GPU | Model |
   |---|---|
   | RTX 50 | NVIDIA's original, signed 310.8 `nvngx_dlssnr.dll` |
   | RTX 20, 30, 40 | ShortFuse's cross-generation 310.8 build, from the RenoDX Discord thread linked in that guide |

   The release notes say 310.9 models work too, and add the smaller, cheaper
   model as Preset B.
3. Check the file against the guide's SHA-256 before using it, because the
   RTX 20–40 build is modified and its NVIDIA signature no longer validates:

   ```powershell
   Get-FileHash .\nvngx_dlssnr.dll -Algorithm SHA256
   ```

4. Start the client and open the menu with **Insert**. The Neural Rendering
   page should show **Model backend: NVIDIA NGX**.

`Dxgi=false` is already the default on NVIDIA.

### AMD and Intel GPUs

1. From the release's `Optional-Vendor-Neutral-DLSS5-Forwarder.zip`, copy
   `nvngx.dll_dlssnr.dll` and `nr_port.ini` over the default forwarder. This
   version runs the model in DirectML instead of NGX.
2. Add the **310.9** `nvngx_dlssnr.dll`; the vendor-neutral forwarder doesn't
   work with 310.8.
3. The menu's Neural Rendering page should show **Model backend:
   vendor-neutral port**, and `nr_port.log` should say `network: small`.
   `network: main` means it found an older model.

## Checking it works

Set `LogToFile=true` under `[Log]` in `OptiScaler.ini`. In `OptiScaler.log`:

- `WrappedIDXGISwapChain4 ... created`: the swapchain is hooked. `Overlay call!`
  for it means the SDL fork is not in the build.
- `Native motion frames: of N presents ... flow valid ... (matched with depth M)`:
  motion is estimated, and M of those frames used the scene's depth.
- `DLSS-NR forwarder loaded from ... (vendor-neutral port ...)` on AMD or Intel.
- `DLSS-NR unavailable: nvngx_dlssnr.dll was not found`: the model is missing.
- `This screen colour format is not supported.`: the swapchain is BGRA, so the
  fork is not in the build.

## Limits

- No super resolution. The client still renders at full resolution, and
  OptiScaler's upscaler runs at the same size, so NR changes how the picture
  looks but doesn't raise FPS. Upscaling needs the in-engine work in
  [Next step](#next-step).
- Direct3D 12 only. On Vulkan, the finished-picture mode cannot run, and the
  SDL fixes above don't apply.
- With `Backend=d3d12`, the frame cap and vsync don't limit FPS correctly. That
  is the reason Windows defaults to Vulkan.
- The HUD and text are processed along with the 3D scene.
- Don't ship the mod or the model with client releases. The model is
  NVIDIA-derived and only available from separate sources.

## Next step

An in-engine DLSS call would keep the HUD out of NR and replace estimated
motion with the engine's own. With a render scale below 1 it also brings
super resolution: the scene renders at lower resolution and DLSS upscales it. The client would call DLSS (NVIDIA's NGX SDK)
through the native handles above, between the 3D scene pass and the interface,
in `ReplayPostProcessedFrame`. That needs:

- Sub-pixel jitter on the main-scene projection.
- A motion vector pass (camera motion first, from depth and the previous frame's
  view-projection).
- A depth buffer created with `SDL_GPU_TEXTUREUSAGE_SAMPLER`.
- MSAA off while DLSS is on.

OptiScaler intercepts that DLSS call and runs NR around it. NGX's DLSS itself
only runs on NVIDIA RTX cards.
