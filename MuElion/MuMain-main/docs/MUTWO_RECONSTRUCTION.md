# MuTwo / MuClient reconstruction

This branch reconstructs the observable VDraven/MuClient runtime architecture
on top of MuElion/MuMain-main. Only this subtree is modified.

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

The Debug executables preserve private source/type information for
`client/render/FrameTape.*`, `client/session/SessionRender.cpp`,
`LegacyRenderFacade`, `SessionLegacyCalls`, the RenderTape structures,
RmlUi integration, target transfers, and several GPU buffer layouts.

## Implemented

### Runtime / configuration

- SDL GPU backend selection through `SDL_GPU_DRIVER`
- Vulkan / Direct3D12 public backend contract
- VSync and FPS limit contract
- `RenderPipeline=0` immediate fallback
- worker-backed `RenderPipeline=1`
- `SessionWorkerCount`
- `SharedAssetIdleSeconds`
- independent `RmlScale` and `ControlUIScale`

### Render tape

- frame/session identity and timing
- worker job accounting
- World / UI / Overlay passes
- ordered draw and clear commands
- 2D, 3D, text and skinned draws
- exact Debug-observed rejection reason strings
- `render-tape-failures.log` line format
- logical texture assets and lifetime retention
- logical geometry leases and trusted draw validation
- indexed-triangle and trail-sample reservations
- target copy/download commands
- asynchronous RGBA8 frame readback and sub-rectangle crop
- completed target download retrieval

### LegacyRenderFacade

Every named `LegacyRenderFacade` method currently recovered from the Debug
signature inventory has a declaration and implementation. The audit covers
91 named methods, including:

- state / matrix / fixed-function compatibility
- client arrays and triangle classification
- `WriteTriangles` / `WriteTriangleFan`
- geometry / terrain / grass paths
- trail / particle / quad / sprite instances
- BMD bone palette, geometry, rigid instances and shadow geometry
- Debug-matched BMD scale routing and bit-6 rigid-transform override
- texture definition / frame-only quad
- copy target / RGBA target download

Some instance/terrain paths currently use a CPU-expanded compatibility path
where the exact private structured-buffer submission has not yet been
reconstructed. Their public behavior is represented, but they are not claimed
to be instruction-identical to MuTwo.

### Session rendering

- all 36 named `SessionLegacyCalls` methods recovered from the Debug inventory
- `SessionRenderUnit::BeginRenderTapePass`
- exact recovered `SessionRenderUnit::RenderPointRotate` signature
- modern UI preparation hook
- frame identity propagation
- tape finalize/replay before renderer presentation

### GPU layouts recovered from Debug/shader evidence

- `RenderTapeBoneMatrix`
- `RenderTapeTerrainCell`
- private BMD constants size: 232 bytes
- private terrain constants size: 36 bytes
- shader-visible vertex constants: 384 bytes
- rigid instance: 96 bytes
- trail instance: 96 bytes
- quad instance: 96 bytes
- particle instance: 64 bytes
- terrain instance: 16 bytes

See `MUTWO_DEBUG_SYMBOLS.md` for evidence and caveats.

### RmlUi

- RmlUi 6.3-compatible `TapeRenderInterface`
- `RmlUiRuntime`
- `MuSystemInterface`
- `RmlHudMapViewport`
- recovered `map-view` and `map-viewport` element aliases
- recovered `gfx-tint` decorator and nested SpriteDecorator, including
  Debug-matched base texture storage and rectangle/tint member layout
- recovered `RmlUiDesign::Parser` for `mu-design-*` meta constants
- reconstructed `RmlMuButton` listener, atomic click latch, visibility/
  enabled state and CSS synchronization
- reconstructed `RmlMuMovablePanel` drag listener, bounds clamp, position
  state and `left/top` CSS synchronization
- reconstructed `RmlMuOptionStepper` with embedded previous/next buttons,
  exact key navigation and one-shot optional change delivery
- RmlUi `CompileShader / RenderShader / ReleaseShader` support for `gfx-tint`
- cached CPU-equivalent `textureTint` transform for RmlUi-owned textures
- modern UI called from the live frame lifecycle
- immediate-mode fallback when `RenderPipeline=0`
- deterministic FreeType and LunaSVG build dependencies
- neutral reconstructed HUD bootstrap:
  `Data/UI/PC/HUD/main_frame.rml`
- reconstructed stylesheet bootstrap:
  `Data/UI/PC/ui_assets.rcss`
- lazy RML document host with load/show/hide lookup by recovered path
- neutral independently-authored skeletons for every fully-qualified RML path
  currently confirmed by the Debug executable
- Debug-derived RML document inventory in `MUTWO_RML_DOCUMENTS.md`

No VDraven RML/RCSS contents are copied. The public repository does not expose
a license file, so UI documents are reconstructed independently.

## Remaining validation / parity work

- [ ] Full clean Windows x64 compile
- [ ] Full clean Windows x86 compile
- [ ] Run-time validation against Main-x64-Debug.exe
- [ ] Run-time validation against Main-x86-Debug.exe
- [ ] Vulkan visual/state parity pass
- [ ] Direct3D12 visual/state parity pass
- [ ] Feed more real scene-preparation work into SessionRender workers where
      thread ownership permits
- [ ] Replace remaining CPU-expanded instance/terrain compatibility paths with
      the exact private storage-buffer path as more structure offsets are
      recovered
- [ ] Replace the neutral RML skeletons screen-by-screen with behaviorally
      reconstructed UI from observable behavior and the existing MuMain UI

## Current confidence boundary

The named RenderTape/LegacyRenderFacade/SessionLegacyCalls surface recovered
from the Debug executables is now represented in source. This does **not** mean
that the complete private source was recovered byte-for-byte.

The remaining uncertainty is primarily:

1. private field semantics not preserved by compilation,
2. exact native structured-buffer batching for some instance/terrain modes,
3. the contents/behavior of individual RmlUi documents,
4. whole-project compile/link/runtime validation.

Do not mark this reconstruction as fully validated until clean x86/x64 builds
and differential runtime tests succeed.
