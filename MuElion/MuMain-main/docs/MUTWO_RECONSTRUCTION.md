# MuTwo / MuClient reconstruction

This branch reconstructs the public VDraven/MuClient runtime changes on top of
MuElion/MuMain-main. Only this subtree is modified.

## Confirmed runtime contract

The public MuClient package exposes these performance keys:

- RendererBackend = auto | direct3d12 | vulkan
- VSync = 0 | 1
- RenderPipeline = 0 | 1
- FpsLimit = 0..1000 (0 = uncapped)
- SessionWorkerCount = 2..64
- SharedAssetIdleSeconds >= 0
- RmlScale = 100..200
- ControlUIScale = 100..200

The debug executable also preserves the private source names
`client/render/FrameTape.cpp`, `client/render/FrameTape.h`, and
`client/session/SessionRender.cpp`.

## Implemented in this commit

- Backend selection is wired to SDL GPU through SDL_GPU_DRIVER.
- FpsLimit=0 maps to the existing uncapped target-FPS path.
- FrameTape records frame/session timing and render-job counts.
- SessionRender provides immediate mode and a worker-backed pipeline.
- The main render loop begins/ends SessionRender frames.
- Session workers shut down before GPU resources.
- A config.ini.template contains the reconstructed performance/UI contract.

## Reconstruction status

- [x] Configuration contract
- [x] Renderer backend selection
- [x] FPS contract
- [x] FrameTape base
- [x] SessionRender worker pipeline base
- [ ] Feed real scene preparation jobs into SessionRender
- [ ] Shared asset idle cache integration
- [~] UI scale integration: ControlUIScale drives legacy scalable panels/HUD;
      RmlScale is loaded as a separate factor awaiting the RmlUi layer
- [ ] Binary-differential validation against x86/x64 Debug and Release
- [ ] Vulkan/D3D12 shader/pipeline parity validation


### UI scale contract

`ControlUIScale` is now applied to the existing scalable control/panel/HUD
transforms and font cache sizing. It intentionally does not multiply the raw
640x480 screen-overlay transform, so input/world-overlay coordinates remain
stable.

`RmlScale` is loaded independently through `UI::Scaling::GetRmlUiScale()`.
It is not applied to legacy controls; it is reserved for the RmlUi document
layer, matching the separation exposed by the public MuClient config.
