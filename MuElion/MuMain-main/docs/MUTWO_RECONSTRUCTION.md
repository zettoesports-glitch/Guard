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

- central `RmlPcUiHost` owns the reconstructed PC controllers and coordinates
- `RmlMainFrameLegacyBridge` feeds legacy HP/MP/SD/AG, exact EXP progress,
- `RmlMuHelperLegacyBridge` maps the real `g_MuHelper.GetConfig()` state,
- `RmlInventoryLegacyBridge` projects legacy visibility, repair/shop state,
- `RmlInventoryExtensionLegacyBridge` maps the legacy 0–4 opened bags and
- `RmlPrivateStoreLegacyBridge` projects seller/buyer visibility, shop-open
  state, title/name and the 32-slot inventory through public legacy controls
  128 global-indexed extension slots by occupancy without retaining items
  64 inventory cells and 12 equipment slots into an explicitly loaded modern
  inventory without owning any `ITEM*`
  visibility, assigned attack/buff skills and extra-item filters into an
  explicitly loaded modern helper without sending packets
  poison state, current skill, hotkeys and cooldowns into an explicitly loaded
  modern main frame without auto-enabling it
  initialize/update/resize/shutdown without auto-loading any document
- Windows lifecycle updates the host before the RmlUi context update and
  releases all document owners before `RmlUiRuntime::Shutdown`

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
- reconstructed `RmlMuScrollBar` four-child binding, proportional thumb,
  page clicks, drag navigation and one-shot position requests
- reconstructed `RmlMuSlot` as a `RmlMuButton` derivative with secondary
  mouse latch and exact `icon-frame-N` CSS switching
- reconstructed `RmlChatPanel::Impl` controller with exact document path,
- reconstructed `RmlFriendPanel` main/chat/write/read presentation with
- reconstructed `RmlPartyFrameLayer` with five member slots, proportional
  HP/MP gauges, leader/visibility states, drag/minimize and semantic
  select/leave requests
  recovered design metrics, movable windows, rows, scrollbar and semantic
  one-shot actions while preserving legacy `CUIWindowMgr` ownership
  21 design keys, recovered message-class switch, retained message rows,
  scroll controls and blocked-chat presentation
- Debug-matched chat `EventListener` inheritance, exact runtime maxlengths,
  dynamic row IDs, blocked/chat click dispatch and hover marquee transitions
- recovered chat update-loop marquee dirty/deadline semantics and exact x64
  private `RmlChatPanel::Impl` object size (0xA38)
- read-only legacy chat snapshot bridge preserving `CNewUIChatLogWindow`
  ownership while feeding UTF-8 messages into the modern panel
- reconstructed `RmlDocumentHost` lifecycle wrapper with runtime-dispatched
  release and safe RmlUi deferred document closing
- reconstructed `RmlMainFrameLayer` with exact main-frame document path,
  nine design keys, recovered DOM IDs/state classes, gauges, six skill icons,
  scale layout and seven button intents
- reconstructed dynamic `skill_list_icons.rml` sublayer with Debug-exact
  element creation/id/class/inner-RML sequence and legacy chooser arrangement
- reconstructed top-menu layer with exact document/DOM contract, nine design
  keys, independent option/action enable state and `start`/`stop` helper state
- reconstructed `RmlTooltipLayer` with Debug-exact capacity/id binding,
- reconstructed `RmlMessageBoxPanel` normal/caution layouts, one/two
  buttons, safe text lines, input variants/limits and semantic OK/Cancel
  actions matching the legacy message-box callback boundary
  line style maps, blank/bold generation, alignment/frame/min-width and position
- reconstructed `RmlCharacterFramePanel` with seven design keys, general
  character fields, five stat sections/buttons, Pet/Master/Close intents and
  movable panel presentation
- reconstructed `RmlPetFrameLayer` with nine design keys, five pet rows,
  dual HP bars, minimize action and movable presentation
- reconstructed `RmlPetInfoPanel` with eleven design inputs, two tabs,
  five info rows, progress state, four-row scrollable skill list, drag and close
- reconstructed `RmlCommandWindowPanel` with nine design inputs, eleven
  modern command buttons, retained selection, drag and close intents
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
- [ ] Replace the remaining neutral RML skeletons screen-by-screen with
      behaviorally reconstructed UI from observable behavior and the existing
      MuMain UI (Inventory + Expanded Inventory are now materialized) (Chat, Main Frame, Top Menu, Character/Pet, Command Window, Inventory and MU Helper are now materialized functionally)

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

- reconstructed `RmlSystemMenuPanel` with recovered full/compact geometry,
  independent action visibility, Escape handling and semantic system actions

- reconstructed `RmlServerMessageLayer` safe passive server-text overlay
- reconstructed `RmlStoreLabelLayer` dynamic world names/store/chat layer
  with projected anchors and relationship/reputation presentation state

- reconstructed `RmlMoveCommandPanel` with 12-row move list, five favorite
  rows, recovered design geometry, scrollbar, favorite toggles and semantic
  select/show-map/close actions while leaving warp/network checks game-side

- reconstructed `RmlMasterTreePanel` with all 108 authored slots, recovered
  panel/reference metadata, legacy-derived group/rank/column fallback geometry,
  drag/close/EXP controls and semantic upgrade requests while preserving
  master-skill eligibility/confirmation game-side

- reconstructed `RmlInventoryPanel` with 64 dynamic inventory slots,
  twelve equipment slots, recovered Inventory design geometry, movable frame
  and semantic repair/store/extension/socket/set/close actions while keeping
  item ownership and networking in `CNewUIMyInventory`
- reconstructed `RmlOptionPanel` with checkbox/radio/select controls for
- reconstructed Inventory/Expanded Inventory presentation with Debug-derived
- reconstructed Private Store presentation with exact Debug design keys,
- reconstructed MuHelper presentation over the real `MUHelper::ConfigData`
- reconstructed Login/Login Bottom presentation with Debug-derived anchors,
  two-column bottom controls and one-shot credential/menu/cancel requests
  domain with tabs, ranges, checkbox state, skill slots, extra-item list and
  one-shot save/reset/settings requests
  8x4 slot grid, seller/buyer mode, drop-state colors and one-shot actions
  dimensions, grid/equipment contracts, 64 base slots, 128 extension slots,
  drag/button controls and one-shot requests; legacy item ownership preserved
  legacy and modern options, 0..10 volume meters, 0..4 effect level and typed
  immediate-change actions while keeping GameConfig/SDL/audio/font side effects
  outside RmlUi

- materialized shared `Common/common.rcss` plus common/scrollbar design
  contracts so reconstructed panels no longer depend on a missing shared RCSS

- reconstructed `RmlInventoryExtensionPanel` with four 32-slot bag groups,
  recovered progressive heights/grid metrics, movable presentation and
  semantic slot/close actions while preserving legacy extension ownership

- reconstructed `RmlPrivateStorePanel` with seller/buyer modes, 32-slot
  grid, recovered drop colors, editable shop name and semantic open/close/
  slot actions while keeping price validation and networking in legacy code

- reconstructed `RmlMuHelperPanel` main control surface with three tabs,
  range selectors, major helper toggles, six assigned skills, ten available
  skill choices, extra-item list/scroll and reset/save/close/configure intents
  while preserving legacy MUHelper serialization and game-side behavior

- reconstructed `RmlStoragePanel` with 120-slot 8x15 vault grid, recovered
  panel geometry, Zen/fee/period presentation, lock/extend state and semantic
  vault actions; central host owns it while legacy storage keeps network,
  password and item-movement authority

- reconstructed `RmlStorageExtensionPanel` as the second 120-slot vault
  surface with recovered right-side initial placement and central-host
  lifecycle; global index offset and item movement remain legacy-owned

- reconstructed `RmlStorageSecurityPanel` for storage keypad/password
  documents with externally supplied randomized digit mapping, four-digit PIN,
  masked input, backspace and submit/cancel intents; all validation/network
  ownership remains in legacy vault message-box code

- reconstructed `RmlTradePanel` across trade/trade_confirm documents with
  two 32-slot grids, player/Zen metadata, warning/confirmation presentation
  and semantic close/Zen/confirm/slot actions while legacy trade retains all
  item legality, timers and network requests


- reconstructed `RmlServerSelectPanel` for the public
  `Data/UI/PC/ServerSelect/server_select.rml` contract with dynamic group/
  server rows, load bars, PvP/Non-PvP/Gold presentation and one-shot server
  selection intent
- added `RmlServerSelectLegacyBridge` over the existing
  `CServerListManager` public group/server data; server discovery and
  ownership remain legacy-side, and the modern panel does not initiate the
  network connection itself


- reconstructed `RmlCharacterCreatePanel` for the public
  `Character/character_create.rml` contract, including seven class choices,
  five stat rows, 10-character name input, description, OK/Cancel and
  viewport fitting
- added a read-only `CCharMakeWin::BuildSnapshot` and
  `RmlCharacterCreateLegacyBridge`; class availability, selected class,
  localized labels/stats and visibility remain legacy-authoritative while
  modern Create/Cancel/Class actions remain semantic intents only


- reconstructed `RmlBuffListLayer` over the public
  `HUD/buff_list.rml` contract, with separate buff/debuff containers,
  eight-column retained icons, count/time presentation and click intents
- added read-only `CNewUIBuffWindow::BuildSnapshot` and
  `RmlBuffListLegacyBridge`, preserving the existing tier normalization,
  hidden-buff filtering, buff/debuff ordering, reference counts and remaining
  time from the authoritative legacy buff system
