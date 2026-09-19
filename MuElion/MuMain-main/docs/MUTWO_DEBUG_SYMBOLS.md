# MuTwo Debug symbol map

Source: public VDraven/MuClient x86/x64 Debug executables, build 17 Sep 2026.

This file records names preserved by MSVC debug/assert strings. It is evidence for
the reconstruction and should not be interpreted as the original private source.

## Confirmed private source paths

- `client/render/FrameTape.h`
- `client/render/FrameTape.cpp`
- `client/session/SessionRender.cpp`

## Render rejection strings

The x86 Debug binary contains:

- `invalid-tape-identity`
- `missing-logical-asset`
- `invalid-tape-contents`
- `asset-table-mismatch`
- `unknown`
- `[RenderFrame] rejected reason=%s(%u) session=%llu frame=%llu asset=%llu:%llu`

It also logs render-tape failures as:

`[RenderTape] failure=%u pass=%u source-row=%u order=%llu frame=%llu`

to `render-tape-failures.log`.

## LegacyRenderFacade signatures recovered

```text
BeginPass(RenderTapePass, SessionFogPassConstants const&) noexcept
EndPass() noexcept
Finalize() -> optional<SessionRenderTape>
Begin(LegacyPrimitive) noexcept
End() noexcept
Vertex3(float,float,float) noexcept
Color4(float,float,float,float) noexcept
Normal3(float,float,float) noexcept
TexCoord2(float,float) noexcept
EmitExpandedTriangles(LegacyPrimitive, span<RenderTapeVertex const>) noexcept
EmitPrimitiveDraw(LegacyPrimitive, span<RenderTapeVertex const>) noexcept
WriteTriangles(uint64,Writer) noexcept
    // Debug preserves instantiations for BMD::RenderMesh,
    // AddMeshShadowTriangles and AddClothesShadowTriangles lambdas
WriteTriangleFan(uint64,Writer) noexcept
    // Debug preserves instantiations for RenderFace, RenderFaceAlpha,
    // RenderFaceBlend, RenderFace_After and RenderSpriteUV lambdas
ReserveIndexedTriangles(uint64,uint64)
    -> optional<LegacyRenderFacade::IndexedTriangleReservation>
SetClientArray(RenderClientArraySemantic, span<byte const>, unsigned int,
               RenderClientArrayScalarType, int, bool) noexcept
EnableClientArray(RenderClientArraySemantic) noexcept
DisableClientArray(RenderClientArraySemantic) noexcept
ReadArrayVertices(int,int,span<RenderTapeVertex>) -> bool noexcept
CountClassifiedArrayTriangles(LegacyPrimitive,int,int,uint64&,uint64&) -> bool noexcept
DrawArrays(LegacyPrimitive,int,int) noexcept
DrawGeometry(LogicalGeometryAssetLease const&,unsigned int,unsigned int) noexcept
BuildTrustedGeometryDraw(LogicalGeometryAssetLease const&,unsigned int,unsigned int,
                         TrustedGeometryDraw&) noexcept
AppendTrustedGeometryDrawBatch(span<TrustedGeometryDraw const>) noexcept
DrawTerrainInstances(LogicalGeometryAssetLease const&,unsigned int,unsigned int,
                     RenderTapeTerrainConstants const&) noexcept
DrawGrassGeometry(LogicalGeometryAssetLease const&,unsigned int,unsigned int,uint64) noexcept
BeginQuadInstanceRun() -> uint64
ReserveTrailSamples(uint64)
    -> optional<SessionRenderTapeRecording::TrailSampleReservation>
DrawTrailInstance(LogicalGeometryAssetLease const&,RenderTapeTrailInstance const&,uint64,bool) noexcept
DrawParticleInstance(LogicalGeometryAssetLease const&,RenderTapeParticleInstance const&,uint64) noexcept
DrawQuadInstance(LogicalGeometryAssetLease const&,RenderTapeQuadInstance const&,uint64) noexcept
DrawSpriteInstance(LogicalGeometryAssetLease const&,RenderTapeQuadInstance const&,uint64) noexcept
AppendBoneMatrices(span<RenderTapeBoneMatrix const>,bool) -> optional<unsigned int>
DrawBmdGeometry(LogicalGeometryAssetLease const&,uint,uint,uint,uint,
                RenderTapeBmdConstants const&) noexcept
DrawRigidInstances(LogicalGeometryAssetLease const&,uint,uint,uint,uint,
                   span<RenderTapeRigidInstance const>,RenderTapeBmdConstants const&) noexcept
DrawBmdShadowGeometry(LogicalGeometryAssetLease const&,LogicalGeometryAssetLease const&,
                      uint,uint,uint,uint,RenderTapeBmdConstants const&) noexcept
Sphere(float,unsigned int,unsigned int) -> bool noexcept
ApplyMatrix(array<float,16> const&) noexcept
MatrixMode(LegacyMatrixMode) noexcept
LoadIdentity() noexcept
LoadMatrix(array<float,16> const&) noexcept
MultMatrix(array<float,16> const&) noexcept
Translate(float,float,float) noexcept
Scale(float,float,float) noexcept
Rotate(float,float,float,float) noexcept
Ortho(float,float,float,float,float,float) noexcept
Perspective(float,float,float,float) noexcept
PushMatrix() noexcept
PopMatrix() noexcept
PushAttrib() noexcept
PopAttrib() noexcept
PushClientAttrib() noexcept
PopClientAttrib() noexcept
SetOpaqueState() noexcept
SetAlphaTestState(bool) noexcept
SetAdditiveState() noexcept
SetTextureEnable(bool) noexcept
SetDepthTestEnable(bool) noexcept
SetDepthWriteEnable(bool) noexcept
SetDepthFunc(RenderCompareFunction) noexcept
SetCullEnable(bool) noexcept
SetCullFace(RenderCullFace) noexcept
SetFrontFace(RenderFrontFace) noexcept
SetBlendEnable(bool) noexcept
SetBlendFunc(RenderBlendFactor,RenderBlendFactor) noexcept
SetAlphaTestEnable(bool) noexcept
SetAlphaFunc(RenderCompareFunction,float) noexcept
SetFogEnable(bool) noexcept
SetFogMode(RenderFogMode) noexcept
SetFogColor(array<float,4> const&) noexcept
SetFogRange(float,float) noexcept
SetFogDensity(float) noexcept
SetLightingEnable(bool) noexcept
SetColorMask(bool,bool,bool,bool) noexcept
SetStencilEnable(bool) noexcept
SetStencilFunc(RenderCompareFunction,uint,uint) noexcept
SetStencilOp(RenderStencilOperation,RenderStencilOperation,RenderStencilOperation) noexcept
SetTextureEnvironment(RenderTextureEnvironment) noexcept
SetShadeMode(RenderShadeMode) noexcept
SetLineWidth(float) noexcept
SetPolygonMode(RenderCullFace,RenderPolygonMode) noexcept
SetViewport(RenderTapeRect) noexcept
SetScissorEnable(bool) noexcept
SetScissor(RenderTapeRect) noexcept
SetClearColor(array<float,4> const&) noexcept
SetClearDepth(float) noexcept
SetClearStencilValue(unsigned int) noexcept
Clear(bool,bool,bool) noexcept
ClearStencil(unsigned int) noexcept
DefineTexture2D(LogicalRenderAssetRef,uint,uint,span<byte const>,LegacyPixelFormat,
                RenderAssetRetention,RenderSamplerIntent) noexcept
AppendFrameOnlyTextureQuad(LogicalRenderAssetRef,uint,uint,span<byte const>,
                           RenderSamplerIntent,array<array<float,2>,4> const&) -> bool noexcept
BindTexture(LogicalRenderAssetRef) noexcept
CopyTargetToLogicalTexture(SessionId,SessionGeneration,uint64,RenderTapeRect,
                           LogicalRenderAssetRef) -> bool noexcept
DownloadTargetRgba8(SessionId,SessionGeneration,uint64,uint64,RenderTapeRect,bool,uint64)
                           -> bool noexcept
```

## SessionRender / legacy call signatures recovered

```text
SessionRenderUnit::BeginRenderTapePass(RenderTapePass) noexcept
SessionRenderUnit::RenderPointRotate(int,float,float,float,float,float,float,
                                     float,float,float,float,float,float,int)

SessionLegacyCalls::glBegin(unsigned int) const
SessionLegacyCalls::glVertex3fv(float const*) const
SessionLegacyCalls::glColor3fv(float const*) const
SessionLegacyCalls::glColor4fv(float const*) const
SessionLegacyCalls::glNormal3fv(float const*) const
SessionLegacyCalls::glTexCoord2fv(float const*) const
SessionLegacyCalls::glEnable(unsigned int) const
SessionLegacyCalls::glDisable(unsigned int) const
SessionLegacyCalls::glDepthFunc(unsigned int) const
SessionLegacyCalls::glBlendFunc(unsigned int,unsigned int) const
SessionLegacyCalls::glAlphaFunc(unsigned int,float) const
SessionLegacyCalls::glCullFace(unsigned int) const
SessionLegacyCalls::glFrontFace(unsigned int) const
SessionLegacyCalls::glPolygonMode(unsigned int,unsigned int) const
SessionLegacyCalls::glMatrixMode(unsigned int) const
SessionLegacyCalls::glLoadMatrixf(float const*) const
SessionLegacyCalls::glMultMatrixf(float const*) const
SessionLegacyCalls::glPushAttrib(unsigned int) const
SessionLegacyCalls::glPushClientAttrib(unsigned int) const
SessionLegacyCalls::glViewport(int,int,int,int) const
SessionLegacyCalls::glScissor(int,int,int,int) const
SessionLegacyCalls::glClear(unsigned int) const
SessionLegacyCalls::glStencilFunc(unsigned int,int,unsigned int) const
SessionLegacyCalls::glStencilOp(unsigned int,unsigned int,unsigned int) const
SessionLegacyCalls::glTexEnvi(unsigned int,unsigned int,int) const
SessionLegacyCalls::glTexEnvf(unsigned int,unsigned int,float) const
SessionLegacyCalls::glFogf(unsigned int,float) const
SessionLegacyCalls::glFogfv(unsigned int,float const*) const
SessionLegacyCalls::glFogi(unsigned int,int) const
SessionLegacyCalls::glShadeModel(unsigned int) const
SessionLegacyCalls::glEnableClientState(unsigned int) const
SessionLegacyCalls::glDisableClientState(unsigned int) const
SessionLegacyCalls::glDrawArrays(unsigned int,int,int) const
SessionLegacyCalls::glVertexPointer(int,unsigned int,int,void const*) const
SessionLegacyCalls::glColorPointer(int,unsigned int,int,void const*) const
SessionLegacyCalls::glTexCoordPointer(int,unsigned int,int,void const*) const
```

## Existing MuMain correspondence

The current MuElion/MuMain-main already has `GLCompatShim` and `IMuRenderer`
operations corresponding to most of the recovered session calls. The safe port
strategy is therefore adapter-based: translate the recovered MuTwo facade/tape
contract onto the existing SDL GPU backend instead of replacing SDL GPU.


## Additional Debug-derived reconstruction notes

The x64 Debug executable preserves local-variable names around the array-triangle
classification path: `triangleIndexCount`, `modelViewProjection`, `indices`,
`counts`, and `frontFacing`. Based on those names, the reconstructed
`CountClassifiedArrayTriangles` classifies expanded triangles using the current
model-view-projection transform and front-face winding. The exact private source
layout is still not claimed to be byte-for-byte recovered.


## Embedded vertex-shader layouts recovered

The x64 Debug executable contains the generated Metal/SPIR-V shader source. It
confirms the following GPU buffer layouts and order:

```text
RenderTapeVertexConstants:
  float4x4 modelView
  float4x4 projection
  float4x4 textureMatrix
  float4 bmdScale
  float4 bmdBodyOrigin
  float4 bmdBodyLight
  float4 bmdBaseColor
  float4 bmdLightPosition
  float4 bmdUvAnimation
  float4 bmdChromeLight
  float4 bmdLegacyLight
  uint4  bmdMode
  float4 rigidTransform0
  float4 rigidTransform1
  float4 rigidTransform2

BmdBoneMatrix:
  float4 row0
  float4 row1
  float4 row2

TerrainCell:
  float height
  uint  wall
  float alpha
  float padding
```

The reconstructed C++ now preserves 48 bytes for `RenderTapeBoneMatrix`,
16 bytes for `RenderTapeTerrainCell`, and the **232-byte private**
`RenderTapeBmdConstants` object confirmed by x64 disassembly. Its first
192 bytes are the shader-visible payload; the remaining 40 bytes are retained
as CPU-side metadata. The shader-visible vertex constant payload remains
384 bytes (three 4x4 matrices + the 192-byte BMD prefix).

`LogicalGeometryAssetLease` is currently a functional reconstruction around
owned vertex/index data. Its private original memory-management implementation
is not claimed to be textually identical.


## BMD mode mapping from embedded shader

Further shader analysis confirms:

- `bmdMode.y` is added to vertex bone indices before indexing `bmdBones`.
  Therefore it is a bone-buffer base offset, not one of the four geometry range
  parameters passed to `DrawBmdGeometry`.
- `bmdMode.z` selects the legacy texture-coordinate mode (mesh/chrome family).
- `bmdMode.w` is a bitfield. Observed bits include translate (bit 0),
  lighting (bit 1), UV animation (bit 2), wave deformation (bit 3), the
  alternate/bone-scale transform path (bit 4), terrain-light override
  (bit 5), and rigid-transform override (bit 6).
- When bit 6 is set, the shader does not index `bmdBones`: it uses
  `rigidTransform0/1/2` for both position and normal transforms. The
  reconstruction maps that path onto a one-bone palette for the SDL GPU
  skinning shader and rewrites the copied vertex bone indices to zero.
- Position scale semantics are now matched at the adapter boundary:
  `bmdScale.x` for the normal transform path, `bmdScale.y` for bit 4,
  and `bmdScale.z` for the post-transform translate path.
- Texture-coordinate mode 8 in MuTwo is the fixed normal-derived mapping,
  while the fallback observed for mode 9 is the normal.xy/rest-UV mapping.
  The SDL GPU shader uses the opposite numeric names for those two terminal
  modes, so the facade translates 8/9 rather than changing global renderer
  semantics.
- Shader mode 6 consumes six float4 rows per rigid instance:
  transform0, transform1, transform2, bodyLight+alpha, baseColor, uvAnimation.
  The C++ reconstruction now asserts `sizeof(RenderTapeRigidInstance) == 96`.

Based on the fact that bone base is already carried by `bmdMode.y`, the four
unsigned parameters on `DrawBmdGeometry`/`DrawRigidInstances` are
reconstructed as vertex-start, vertex-count, index-start and index-count. This
is a semantic reconstruction supported by the shader layout, not a claim that
the original parameter names are known.


## Render-tape failure diagnostics

The reconstruction now writes the exact Debug-observed line shape to
`render-tape-failures.log`:

```text
[RenderTape] failure=<n> pass=<n> source-row=<n> order=<n> frame=<n>
```

The file name and format are confirmed from the Debug executable. The current
numeric failure-code mapping is reconstruction-local; `source-row` is kept at
0 until the original request/source-row propagation is recovered. Pass, order
and frame are populated from the live reconstructed tape.


## Exact private-structure sizes from x64 disassembly

Function-body inspection adds evidence that strings/shaders alone cannot show:

- `DrawBmdGeometry` and `DrawRigidInstances` perform `rep movsb` with
  `ECX=0xE8` from the `RenderTapeBmdConstants const&` argument. Therefore
  the private type is exactly **232 bytes** in this build.
- The embedded vertex shader consumes the first **192 bytes** of that BMD
  payload (the fields documented above). The remaining **40 bytes** are
  CPU/pipeline metadata and are currently retained as opaque bytes.
- `DrawTerrainInstances` performs `rep movsb` with `ECX=0x24` from its
  `RenderTapeTerrainConstants const&` argument. Therefore that private type
  is exactly **36 bytes** in the x64 Debug build.

The C++ reconstruction now preserves these exact total sizes while leaving
unknown CPU-only fields explicitly opaque rather than inventing names.


## Instance layouts and run-token semantics recovered

The embedded vertex shader and x64 facade bodies now provide enough evidence
for these layouts:

- `RenderTapeTrailInstance`: 6 x float4 = 96 bytes. Mode 2 indexes the first
  four rows by corner, taking xyz+U; row 4 supplies per-corner V and row 5 color.
- `RenderTapeQuadInstance`: 6 x float4 = 96 bytes. Mode 3 consumes center,
  half-width/half-height + 2D rotation basis, UV rectangle and row 5 color.
- `RenderTapeParticleInstance`: 4 x float4 = 64 bytes. Mode 7 consumes center,
  half-size/rotation, UV rectangle and color.
- `RenderTapeTerrainInstance`: 1 x float4 = 16 bytes, confirmed by the shader's
  one-row-per-instance terrain modes and by RTTI for
  `vector<RenderTapeTerrainInstance>`.

The x64 `BeginQuadInstanceRun` body also confirms the token protocol:
it returns the previous monotonically increasing run counter and stores that
token as the active run. Grass/quad/sprite/particle/trail methods compare the
caller token against the active token and reject stale/zero tokens.

The current C++ implements this protocol. Instance methods use a CPU-expanded
compatibility path on the existing SDL GPU renderer; this preserves functional
draw behavior while the exact private structured-buffer upload path continues
to be reconstructed.


## Target-copy and RGBA8 download request layout

x64 disassembly confirms the reconstructed tape payloads:

```text
CopyTargetToLogicalTexture:
  SessionId              8 bytes
  SessionGeneration      8 bytes
  targetId               8 bytes
  RenderTapeRect        16 bytes
  LogicalRenderAssetRef 16 bytes

DownloadTargetRgba8:
  SessionId              8 bytes
  SessionGeneration      8 bytes
  targetId               8 bytes
  requestId              8 bytes
  RenderTapeRect        16 bytes
  bool                   1 byte
  userToken              8 bytes
```

The facade now validates the active session/generation/target identity before
recording these commands. Full-frame logical texture copies use the SDL GPU
capture texture path. Cropped copies and downloads share a single native RGBA8
readback for the frame; completed downloads retain requestId and userToken.

The private name of the boolean is not present in recovered symbols. It is
currently modeled as row reversal because that matches framebuffer download
behavior, and this inference is explicitly not treated as a recovered original
identifier.


## SessionRenderUnit / triangle-fan helpers

A second pass over Debug signature strings exposed two items that were not in
the initial facade inventory:

- `SessionRenderUnit::RenderPointRotate(...)` with the exact 14-argument
  signature. The reconstruction delegates to the existing MuMain
  `::RenderPointRotate`, which is already tape-aware.
- `LegacyRenderFacade::WriteTriangleFan<uint64, lambda>` template
  instantiations for `RenderFace`, `RenderFaceAlpha`, `RenderFaceBlend`,
  `RenderFace_After`, and `RenderSpriteUV`. The reconstructed helper
  centralizes fan expansion and accepts lambdas that fill or return
  `RenderTapeVertex`.

This closes every named LegacyRenderFacade/SessionRenderUnit method currently
visible in the Debug signature-string inventory.


## Exact template writer semantics from x64 disassembly

Direct disassembly of the instantiated helpers corrected the earlier generic
writer assumption:

- `WriteTriangles(count, writer)` rejects counts not divisible by 3, returns
  true for zero, reserves exactly `count` vertices and invokes the lambda once
  with a 16-byte `std::span<RenderTapeVertex>`.
- `WriteTriangleFan(count, writer)` returns true for zero, rejects 1/2, and
  invokes the lambda once with `std::span<RenderTapeVertex>`.
- The four-vertex fan has a private fast path that reserves 4 vertices + 6
  indices and writes the exact index sequence `0,1,2,0,2,3`.

The reconstruction now follows those semantics rather than invoking the writer
once per vertex.


## Modern UI / RmlUi compatibility

The Debug executable statically contains RmlUi and exposes RTTI for
`UI::Modern::TapeRenderInterface`, `UI::Modern::RmlUiRuntime` and
`UI::Modern::RmlHudMapViewport`. Its observable RenderInterface ABI matches
RmlUi 6.3. The reconstruction pins the stable 6.3 commit
`ba95ffe8bfb6370efb2cdcca927eaad4710c5413` (released 2026-08-22), which
predates the 2026-09-17 MuTwo Debug build.

The Debug executable also includes RmlUi SVG sources and lunasvg RTTI, so the
modern-UI build enables RmlUi's SVG plugin and links lunasvg. FreeType is
enabled for the RmlUi font engine.

`UI::Modern::TapeRenderInterface` now implements:
- compiled indexed geometry,
- tape-backed arbitrary 2D triangle submission,
- premultiplied-alpha blend semantics (ONE / ONE_MINUS_SRC_ALPHA),
- generated RGBA8 textures backed by LogicalRenderAssetTable,
- direct 24/32-bit uncompressed and RLE TGA loading,
- scissor state,
- RmlUi transform + per-draw translation.

RmlUi's SVG plugin handles SVG documents separately through lunasvg, matching
the Debug source inventory. The higher-level `RmlUiRuntime` and
`RmlHudMapViewport` are reconstructed in subsequent commits.


## RmlUiRuntime reconstruction

The Debug RTTI exposes:
- `UI::Modern::RmlUiRuntime`,
- nested `RmlUiRuntime::Impl`,
- anonymous `UI::Modern::MuSystemInterface`,
- `std::shared_ptr<UI::Modern::RmlUiRenderSnapshot>`,
- `RmlUiRuntime::Impl::Execute(std::function<void()>) -> bool`,
- `SessionRenderUnit::SubmitModernUiPreparation(RmlUiRuntime&) -> bool`.

The reconstruction now mirrors that architecture. The runtime owns a RmlUi
context, uses the tape render interface, applies `RmlScale / 100` through
`Context::SetDensityIndependentPixelRatio`, tracks window resize, creates a
render snapshot per preparation call, and is initialized/shut down with the SDL
GPU renderer lifetime.

`SubmitModernUiPreparation` records RmlUi rendering inside a UI tape pass
when no pass is already active. It deliberately does not force rendering from
the main loop until documents are loaded by the modern document hosts.


## RmlHudMapViewport reconstruction

The public MuClient HUD assets contain the custom CSS selector `map-view` and
the Debug executable contains both the literal `map-view` and RTTI for
`Rml::ElementInstancerGeneric<UI::Modern::RmlHudMapViewport>`. The modern
runtime now registers that exact custom element tag.

`RmlHudMapViewport::SetImage(const LogicalRenderAssetMetadata&) -> bool`
matches the Debug signature. It imports the existing logical texture into
`TapeRenderInterface` without taking ownership, creates a
`Rml::CallbackTextureSource`, and renders a resize-aware textured quad through
RmlUi geometry. This matches the Debug RTTI showing a SetImage lambda taking
`const Rml::CallbackTextureInterface&`.


## RmlHudMapViewport element aliases

The Debug executable preserves both literal element names `map-view` and
`map-viewport` next to RTTI for `RmlHudMapViewport`. The reconstructed
RmlUi runtime now registers the same viewport element instancer under both
aliases.


## Recovered gfx-tint decorator

Further x64 Debug disassembly and RTTI expose:
- `UI::Modern::RmlGfxTintDecoratorInstancer`,
- nested `RmlGfxTintDecoratorInstancer::SpriteDecorator`,
- decorator name `gfx-tint`,
- properties `sprite, scale, red, green, blue`,
- defaults `scale=1` and RGB offsets `0`,
- `SpriteDecorator` compiling a RmlUi shader named `gfx-tint` with a
  `Vector4f` parameter named `tint`.

The embedded fragment shader confirms the exact color transform:

```text
rgb = clamp(sampled.rgb * textureTint.x + textureTint.yzw, 0, 1)
alpha = sampled.a
output = vertexColor * float4(rgb, alpha)
```

Property mapping is therefore `textureTint = (scale, red/255, green/255,
blue/255)`.

The reconstructed TapeRenderInterface now implements the same RmlUi
`CompileShader / RenderShader / ReleaseShader` contract. Until the complete
private fragment pipeline is ported to the checked-in SDL GPU shader blobs,
`gfx-tint` materializes a cached derived RGBA8 texture from the retained
RmlUi CPU texture snapshot. This is mathematically equivalent for the recovered
gfx-tint operation and leaves sampled alpha unchanged. Imported/captured
textures without a CPU snapshot fall back to the untinted texture rather than
disappearing.


## RmlUiDesign metadata parser

MSVC RTTI in the x64 Debug executable exposes
`UI::Modern::RmlUiDesign::Parser` deriving from `Rml::BaseXMLParser`.
The recovered vtable matches the RmlUi 6.3 base parser order:
destructor, `HandleElementStart`, `HandleElementEnd`, and `HandleData`.

Direct disassembly of `HandleElementStart` confirms the parser only handles
`meta` elements. It reads the `name` attribute, requires the exact prefix
`mu-design-`, strips that prefix, reads the `content` attribute, and stores
the key/value pair in a parser-owned map. The recovered `HandleElementEnd`
and `HandleData` bodies are empty.

The x64 deleting-destructor passes a private object size of `0x138`; the
reconstruction does not claim identical STL container layout, but preserves
the observable parsing contract.


## Exact gfx-tint SpriteDecorator member layout

A direct pass over the x64 Debug bodies corrects one detail from the first
functional reconstruction. `SpriteDecorator` does not store an independent
`Rml::Texture` member. Its constructor calls `Rml::Decorator::AddTexture`,
and both `GenerateElementData` and `RenderElement` retrieve texture index 0
through `Decorator::GetTexture`.

The observed x64 member offsets are consistent with that base-class storage:

```text
SpriteDecorator + 0x38 : Rml::Rectanglef sprite rectangle
SpriteDecorator + 0x48 : Rml::Vector4f textureTint
```

The reconstruction now follows this layout: texture ownership/reference is kept
by the RmlUi Decorator base, followed only by the recovered rectangle and tint
members.


## RmlMuButton listener and state machine

RTTI exposes `UI::Modern::RmlMuButton::Listener`. Its x64 object is 24 bytes:
the RmlUi `EventListener` base occupies the leading portion and the owner
pointer is stored at `+0x10`. `ProcessEvent` is a one-hop forwarder to the
owner.

The non-polymorphic `RmlMuButton` body is also 24 bytes in the x64 Debug
build. Direct function-body analysis recovers this layout and behavior:

```text
+0x00 unique_ptr<Listener>
+0x08 Rml::Element* bound element
+0x10 atomic<bool> enabled        (default true)
+0x11 atomic<bool> visible        (default true)
+0x12 atomic<bool> clicked        (default false)
+0x13 optional<bool> applied-enabled
+0x15 optional<bool> applied-visible
```

Binding first detaches any previous element, then attaches the listener to the
exact event string `click`. Unbinding removes the same listener and clears the
latched click. The click handler performs acquire loads of enabled/visible and
sets the click latch with release ordering only when both are true. The
consume operation uses an atomic exchange with acq_rel ordering.

The recovered visual synchronization maps state to RmlUi as follows:

```text
visible=false -> class "mu-hidden" enabled
enabled=false -> class "disabled" enabled
enabled=true  -> pointer-events: auto
enabled=false -> pointer-events: none
```

The Debug body also contains an ancestor walk which returns true only when the
control is enabled+visible and the supplied element is the bound element or one
of its descendants. The reconstruction exposes semantic method names for these
non-exported functions; those names are not claimed to be the original private
identifiers.


## RmlMuMovablePanel drag controller

RTTI exposes `UI::Modern::RmlMuMovablePanel::Listener`; the listener stores
its owner at x64 offset `+0x10` and forwards `ProcessEvent(Event&)`
unchanged.

The owner constructor and event bodies recover the following x64 layout:

```text
+0x00 unique_ptr<Listener>
+0x08 Rml::Element* panel
+0x10 Rml::Element* drag handle
+0x18 float container width
+0x1c float container height
+0x20 float panel width
+0x24 float panel height
+0x28 float parent/origin x
+0x2c float parent/origin y
+0x30 float position x
+0x34 float position y
+0x38 float drag mouse offset x
+0x3c float drag mouse offset y
+0x40 bool dragging
+0x41 bool position changed
```

Binding attaches the same listener to `dragstart`, `drag`, and `dragend`
on the handle. On drag start, the recovered code reads float event parameters
`mouse_x` and `mouse_y` and stores mouse-minus-panel offsets. While dragging,
it derives the new absolute position from those parameters, clamps each axis to
`0..max(0, container_size-panel_size)`, and writes the panel properties
`left` and `top` using the exact format `%.3fpx` after subtracting the
stored parent/origin offset. Applying CSS sets the position-changed latch.

The reconstruction preserves these behaviors while giving semantic names to
non-exported methods whose original private identifiers are not present.


## RmlMuOptionStepper reconstruction

RTTI exposes `UI::Modern::RmlMuOptionStepper::Listener`. Direct x64
disassembly recovers the control layout and behavior:

```text
+0x00 unique_ptr<Listener>
+0x08 Rml::Element* root
+0x10 RmlMuButton decrement
+0x28 RmlMuButton increment
+0x40 int current
+0x44 int maximum
+0x48 bool enabled
+0x4c optional<int> pending
```

Binding removes any previous `keydown` listener, attaches the listener to the
new root, and binds the two embedded `RmlMuButton` controls. State updates
enable the decrement button only for `enabled && current > 0` and the
increment button only for `enabled && current < maximum`; disabling clears
the pending optional.

The recovered key parameter is exactly `key_identifier`. RmlUi 6.3 enum
values in the Debug switch map to:

- `KI_TAB`: stop propagation without changing the value
- `KI_END`: set candidate to `maximum`
- `KI_HOME`: set candidate to 0
- `KI_LEFT`: decrement
- `KI_RIGHT`: increment

Recognized value-changing keys clamp to `[0, maximum]` when enabled and then
call `Event::StopPropagation()`. Button clicks feed the same pending value.
The consumption method updates the local current value and returns
`std::exchange(pending, std::nullopt)`, giving one-shot change delivery.

The reconstruction uses semantic public method names because private
non-exported identifiers are not present in the executable.


## RmlMuScrollBar reconstruction

RTTI exposes `UI::Modern::RmlMuScrollBar::Listener`; its virtual
`ProcessEvent(Event&)` forwards through the owner pointer stored at listener
offset `+0x10`. The recovered scrollbar object begins with five element
pointers:

```text
+0x00 unique_ptr<Listener>
+0x08 root
+0x10 track
+0x18 increment/down
+0x20 decrement/up
+0x28 thumb
+0x30 current position (size_t)
+0x38 maximum position (size_t)
+0x40 visible/page fallback amount (size_t)
...
+0x50 explicit page step (size_t)
...
+0xa0 optional<size_t> requested position
+0xb0 drag scale
+0xb4 drag-start mouse_y
+0xb8 drag-start position
+0xc0 dragging
```

Binding verifies four children and assigns indices 0..3 to track, increment,
decrement and thumb. It registers `click` on the first three and
`dragstart`, `drag`, `dragend` on the thumb. Unbinding removes the same
listeners.

The event body is recovered directly:

- decrement click requests `position - 1` when position > 0;
- increment click requests `position + 1` when position < maximum;
- track click reads `mouse_y`, compares against the thumb center, and moves by
  the explicit page step or `max(1, visibleAmount)`;
- dragstart stores `mouse_y` and the starting position;
- drag converts vertical pixel movement into logical position movement and
  clamps to `0..maximum`;
- dragend clears the dragging latch;
- handled events call `Event::StopPropagation()`.

The private position setter writes a 16-byte optional-like object near +0xa0,
showing that interaction requests are delivered separately from externally
owned scrollbar state. The reconstruction therefore exposes
`ConsumeRequestedPosition()` rather than mutating application state behind
the owner's back.

The recovered layout helpers also show proportional thumb sizing and position
mapping. The reconstruction applies equivalent `top`, `height`, `display`
and disabled/pointer-event state through RmlUi properties. Public method names
are semantic reconstructions, not claims about stripped private identifiers.


## RmlMuSlot reconstruction

The x64 class layout shows `RmlMuSlot` deriving directly from the
0x18-byte reconstructed `RmlMuButton`. Its additional fields are:

```text
base +0x00..+0x17  RmlMuButton
+0x18              unique_ptr<RmlMuSlot::Listener>
+0x20              Rml::Element*
+0x28              atomic<bool> secondaryClicked
+0x2c              int iconFrame
```

Binding first resets previous state, calls the button binding path for the same
element, and then registers an additional `mouseup` listener. Unbinding
removes the current `icon-frame-N` class when nonzero, removes `mouseup`,
unbinds the button base, zeroes the frame and clears the atomic latch.

The listener reads the exact event parameter `button`; when its integer value
is 1 it performs an atomic store of true into the byte at +0x28. A separate
method uses atomic exchange(false) to consume that latch once. Hiding/clearing
the slot also clears the secondary-click state.

Direct disassembly of the frame setter shows the exact class-name construction
`icon-frame-` + decimal frame number. Changing frames removes the old class,
stores the new integer at +0x2c, and applies the new class when nonzero.

The reconstruction exposes this as `ConsumeSecondaryClicked()` and
`SetIconFrame()`; these semantic names are not claimed to be the stripped
private source identifiers.


## RmlDocumentHost reconstruction

The Debug executable preserves the exact types
`UI::Modern::RmlDocumentHost`, nested `RmlDocumentHost::Impl`, and a
concrete lambda type created inside `RmlDocumentHost::Impl::Release()`. That
lambda is wrapped by `std::_Func_impl_no_alloc<..., void>`, providing strong
evidence that release work is dispatched through the already recovered
`RmlUiRuntime::Impl::Execute(std::function<void()>)` execution boundary.

Private field names and the full host method inventory are not preserved, so
the reconstruction does not claim an exact private layout. It implements the
observable document lifecycle needed by the recovered panel architecture:

- own a document path and non-owning `ElementDocument*`;
- lazy load through `RmlUiRuntime`;
- show/hide by path;
- release on the runtime execution boundary;
- discard the pointer immediately after release.

RmlUi 6.3 confirms that `ElementDocument::Close()` unloads the document and
defers actual destruction until the next `Context::Update()`. The runtime now
adds `ReleaseDocument(path)`, erasing the host map entry before calling
`Close()` so no stale pointer can be returned by later lookup.

The lambda is deliberately located in the reconstructed
`RmlDocumentHost::Impl::Release()`, matching the RTTI evidence. This is an
architectural/behavioral reconstruction; unknown private identifiers and
memory offsets are not invented.


## RmlChatPanel reconstruction

The x64 Debug binary exposes RTTI for
`UI::Modern::PC::Chat::RmlChatPanel::Impl`, confirming a pimpl-backed chat
panel in the nested `UI::Modern::PC::Chat` namespace.

Direct disassembly at the chat static initializer loads exactly 21 design-key
strings, then associates them with the exact document path
`Data/UI/PC/Chat/chat.rml`. The recovered keys are:

```text
RmlChatPanel-ViewHeights
RmlChatPanel-ResizeViewportHeight
RmlChatPanel-SmallStageScale
RmlChatPanel-LargeStageBottom
RmlChatPanel-SmallStageBottom
RmlChatPanel-ViewWidth
RmlChatPanel-RowHeight
RmlChatPanel-EditingExtraHeight
RmlChatPanel-ViewHeightMargin
RmlChatPanel-MenuTop
RmlChatPanel-InputTop
RmlChatPanel-ScrollDownBottom
RmlChatPanel-MarqueeSecondsPerPixel
RmlChatPanel-ShadowOffsetX
RmlChatPanel-AlphaStep
RmlChatPanel-AlphaMaximum
RmlChatPanel-AlphaMinimum
RmlChatPanel-DefaultAlpha
RmlChatPanel-RmlBlockedChatWidth
RmlChatPanel-RmlBlockedChatHeight
RmlChatPanel-RmlBlockedChatVisibleRows
```

The public MuClient chat asset provides observable metadata values and DOM IDs.
The local reconstruction uses those observable contracts but independently
authors the RML/RCSS content.

A separate x64 switch at `0x1405361d0` recovers the exact message CSS-class
mapping used by the panel presentation layer:

```text
1 -> whisper
2 -> system
3 -> error
4 -> party
5 -> guild
6 -> union
7 -> gm
8 -> gens
default -> normal
```

The reconstructed `RmlChatPanel::Impl` now owns a `RmlDocumentHost`, binds
the recovered chat DOM IDs, reuses the reconstructed `RmlMuButton`,
`RmlMuScrollBar` and `RmlMuMovablePanel` controls, renders retained message
rows, applies proportional scrolling, cycles the recovered view-height and
alpha design values, and manages the blocked-chat presentation/request state.

The bridge to the legacy `CNewUIChatLogWindow` message vectors and game
network/input submission remains a separate integration step. Keeping that
bridge separate prevents the presentation reconstruction from inventing
private ownership relationships that have not yet been confirmed in the
Debug executable.


### Chat Impl vtable and event dispatch

A second x64 RTTI/vtable pass confirms that
`UI::Modern::PC::Chat::RmlChatPanel::Impl` derives directly from
`Rml::EventListener`. Its vtable points the `ProcessEvent(Event&)` slot
through the thunk at `0x140051ed3` to the recovered body at
`0x14056beb0`.

The bind routine stores the primary DOM pointers at these confirmed x64
offsets:

```text
Impl +0x600 chat-panel
Impl +0x608 chat-view
Impl +0x610 chat-title
Impl +0x618 chat-background
Impl +0x620 chat-messages
Impl +0x628 chat-scrollbar
Impl +0x630 chat-scroll-down
Impl +0x638 chat-menu
Impl +0x640 chat-menu-buttons
Impl +0x648 chat-input-area
Impl +0x650 chat-input       (ElementFormControlInput*)
Impl +0x658 chat-whisper     (ElementFormControlInput*)
Impl +0x660 blocked-chat
Impl +0x668 blocked-chat-drag
Impl +0x670 blocked-chat-list
Impl +0x678 blocked-chat-scrollbar
Impl +0x680 blocked-chat-input (ElementFormControlInput*)
```

The Debug then writes the exact RmlUi input attribute
`maxlength`: 89 (0x59) for `chat-input`, 10 (0x0a) for
`chat-whisper`, and 10 for `blocked-chat-input`. The reconstruction now
applies the same runtime attributes.

The recovered event registration is also exact:

```text
chat-panel    -> click
chat-messages -> mouseover
chat-messages -> mouseout
```

`ProcessEvent` dispatches mouseover/mouseout to a marquee helper. That
helper walks ancestors until class `chat-row`, obtains child 0/1
(shadow/line), computes positive text overflow, and applies the exact
transition format `left %.3fs linear`. Duration is
`RmlChatPanel-MarqueeSecondsPerPixel * overflow`; the line moves to
`-overflow` and the shadow to
`RmlChatPanel-ShadowOffsetX - overflow`. On mouseout both transitions are
set to `none` and the local `left` properties are removed.

The root click handler walks element ancestors and recognizes the exact dynamic
ID prefixes `blocked-user-` and `chat-row-`. Blocked IDs parse the decimal
suffix and set the selected blocked-list index. Chat-row IDs validate the
decimal suffix against the retained message vector and forward the selected
message to a private callback stored by the panel. The reconstruction models
that observable callback intent as a one-shot whisper-target request rather
than inventing the original callback type.


## Legacy chat snapshot bridge

The existing MuMain `CNewUIChatLogWindow` remains the authoritative owner of
chat messages and filtering/network behavior. To connect that proven game-side
logic to the reconstructed RmlUi panel without duplicating ownership, the
branch now exposes a read-only `ChatLogSnapshot` built from
`m_vecAllMsgs`.

The snapshot copies only value data (id, text, type, visible-line metadata,
background alpha and show/frame flags); it never exposes or transfers the
legacy `CMessageText*` pointers. `RmlChatLegacyBridge` converts wide text to
UTF-8, maps the legacy message enum onto the Debug-recovered
`RmlChatPanel::MessageKind`, fingerprints snapshots to avoid unnecessary DOM
rebuilds, and feeds the modern presentation.

The bridge is intentionally not made the source of truth for `AddText`,
filter commands or network submission. Those remain legacy-owned until the
Debug ownership/callback graph for the higher-level chat host is fully
recovered.


### Chat update loop, dirty latch and exact Impl size

The x64 deleting destructor for
`UI::Modern::PC::Chat::RmlChatPanel::Impl` passes **0xA38** as the object
size. This is the exact private object size in `Main-x64-Debug.exe`; the
reconstruction does not force the same size because standard-library member
layouts are implementation-specific.

The recovered constructor initializes the retained-message/list state around
`+0x580/+0x5a0`, clears state through `+0x5e8`, sets the blocked selection
at `+0x5f4` to -1, clears the interaction dirty byte at `+0x5f0`, clears
the monotonic deadline at `+0x5f8`, and null-initializes the DOM pointer
range `+0x600..+0x680`.

Direct disassembly of the private update body beginning at
`0x140568260` gives the meaning of the last two fields:

- `+0x5f0` is tested as a dirty/change latch;
- `+0x5f8` is compared against the current monotonic millisecond tick;
- even when no layout/message/control state changed, the update remains active
  while `current_tick < deadline`.

This matches the hover-marquee body, which writes a future deadline after
starting the `left %.3fs linear` transition and clears the deadline on
mouseout. The reconstruction now mirrors that observable behavior with an
interaction dirty latch plus `std::chrono::steady_clock` deadline, without
claiming the original private clock type.


## RmlMainFrameLayer reconstruction

The x64 Debug executable exposes the exact main-frame design-key set and
document path:

```text
Data/UI/PC/HUD/main_frame.rml
RmlMainFrameLayer-ResizeStageHeight
RmlMainFrameLayer-SmallStageScale
RmlMainFrameLayer-ArtWidth
RmlMainFrameLayer-ArtHeight
RmlMainFrameLayer-HotSelectionX
RmlMainFrameLayer-HotSelectionStep
RmlMainFrameLayer-MarbleHeight
RmlMainFrameLayer-LegacyReferenceWidth
RmlMainFrameLayer-LegacyReferenceHeight
```

Direct x64 analysis of the DOM binding body at `0x140665830` recovers the
main-frame element pointer sequence beginning at object offset +0x260:

```text
+0x260 main-frame
+0x268 main-shell
+0x270 main-hp-background
+0x278 main-hp-clip
+0x280 main-hp
+0x288 main-mp-background
+0x290 main-mp-clip
+0x298 main-mp
+0x2a0 main-sd-clip
+0x2a8 main-sd
+0x2b0 main-ag-clip
+0x2b8 main-ag
+0x2c0 main-exp-clip
+0x2c8 main-exp
+0x2d0 main-exp-frame
+0x2d8 main-hp-text
+0x2e0 main-mp-text
+0x2e8 main-sd-text
+0x2f0 main-ag-text
+0x2f8 main-exp-current
+0x300 main-exp-next
+0x308 main-exp-slash
+0x310 main-exp-page
+0x318 main-current-selection
+0x320 main-hot-selection
+0x328 main-skill-page-labels
+0x330 main-skill-page-toggle
```

The same bind body resolves six dynamic `main-skill-icon-N` elements. The
Debug also contains the six exact main-button IDs:
`main-button-shop`, `main-button-character`, `main-button-inventory`,
`main-button-quest`, `main-button-community`, and
`main-button-system`.

The recovered layout body writes the element properties `left`, `top`,
`width`, `height`, and writes the root transform with the exact formatting
string `scale(%.6f)`. The reconstruction retains that format and drives its
small-stage scale from the recovered design threshold.

State-class behavior is directly observable in x64 bodies:

- HP fill toggles `normal` / `poisoned`;
- experience fill selects `normal`, `master`, or `fourth`;
- skill-page labels toggle `page-one` / `page-two`;
- skill-page toggle switches `to-first` / `to-second`;
- the toggle's enabled state is synchronized independently.

`RmlMainFrameLayer` is reconstructed as a presentation controller. It owns
the RML document host, retained DOM pointers and seven button wrappers, accepts
a presentation-neutral HUD state snapshot, updates gauges/text/selection/skill
cooldowns, and exposes button intent through a one-shot `Action`. The
existing legacy `CNewUIMainFrameWindow` remains the source of game rules and
network/UI toggles until a separate read-only bridge is added and validated.


## Top menu layer reconstruction

The Debug executable confirms the full document path
`Data/UI/PC/HUD/top_menu.rml`, the exact nine design keys
`TopMenu-ResizeStageHeight`, `TopMenu-SmallStageScale`,
`TopMenu-FrameWidth`, `TopMenu-FrameHeight`,
`TopMenu-OptionRect`, `TopMenu-ActionRect`,
`TopMenu-ReferenceWidth`, `TopMenu-ReferenceHeight`, and
`TopMenu-TooltipOffset`.

Direct DOM binding disassembly at `0x140668a0a` recovers the eight element
pointers in order:

```text
+0x08 top-menu
+0x10 top-menu-wing-back
+0x18 top-menu-wing-controls
+0x20 top-menu-map-frame
+0x28 top-menu-map-name
+0x30 top-menu-position
+0x38 top-menu-option
+0x40 top-menu-action
```

The state synchronization body around `0x140659895` gives three independent
booleans in the incoming state:

- option-button enabled;
- helper/action running state;
- action-button enabled.

The running boolean controls the exact mutually exclusive CSS classes
`start` and `stop` on `top-menu-action`. The enabled booleans are
forwarded to the two embedded button controls independently.

The reconstruction implements `RmlTopMenuLayer` as a presentation controller
with map name/coordinates, recovered scaling design inputs, two embedded
`RmlMuButton` controls, exact start/stop class behavior, and one-shot
`OpenOptions` / `ToggleHelper` actions. Game-side MU Helper behavior stays
outside this layer.


## RmlCharacterFramePanel reconstruction

The public MuClient character-frame document and Debug string inventory expose
the exact modern document path and seven design keys:

```text
Data/UI/PC/Character/character_frame.rml
RmlCharacterFramePanel-PanelWidth
RmlCharacterFramePanel-PanelHeight
RmlCharacterFramePanel-ReferenceWidth
RmlCharacterFramePanel-ReferenceHeight
RmlCharacterFramePanel-GfxStageWidth
RmlCharacterFramePanel-InitialX
RmlCharacterFramePanel-InitialY
```

The observable DOM contract contains general level/class/server/experience/
point fields, five stat groups, five `character-stat-button-N` controls,
`character-pet`, `character-master`, `character-drag`, and
`character-close`.

The existing MuMain `CNewUICharacterInfoWindow::BtnProcess()` remains the
authoritative game-rule boundary: stat increases send
`SendIncreaseCharacterStatPoint`, pet/master buttons toggle their legacy
interfaces, and close hides the character interface. The reconstructed modern
panel deliberately does not duplicate those side effects. It exposes one-shot
presentation intents instead:

- `IncreaseStat(index)`
- `OpenPet`
- `OpenMaster`
- `Close`

The panel owns the RML document host, binds the exact observable IDs, reuses
`RmlMuButton` and `RmlMuMovablePanel`, supports the fifth charisma/
leadership row only when the supplied state marks it visible, and consumes a
presentation-neutral state containing labels, values and detail text.

The independently-authored RCSS uses the recovered 327x639 authored panel
dimensions and initial 639,40 position. When a viewport cannot contain those
authored coordinates, the reconstruction uniformly scales the panel to fit,
while preserving logical drag bounds. This layout policy is compatibility
behavior and is not claimed to reproduce an unrecovered private transform
formula instruction-for-instruction.


### Dynamic skill-list icon document

The main-frame subsystem also owns the secondary document
`skill_list_icons.rml`. Direct x64 disassembly recovers the container id
`skill-list-icons` and a dynamic icon-construction function beginning at
`0x1406743f0`.

That function receives an icon count, clears the previous retained collection,
then loops exactly that count. For each index it:

1. calls `ElementDocument::CreateElement("div")`;
2. builds the exact id prefix `skill-list-icon-` + decimal index;
3. applies class `mu-skill-icon`;
4. sets the exact inner RML
   `<div class="skill-sheet"/><div class="skill-cooldown"/>`;
5. appends the new element to `skill-list-icons`;
6. stores the resulting element in an internal retained collection.

A separate document/update body stores the resolved `skill-list-icons`
element at object offset +0x28 and keeps a retained collection beginning at
+0x30. The surrounding object is non-polymorphic, so no reliable private class
name survives RTTI; the reconstruction therefore models this as a private
sublayer of `RmlMainFrameLayer` rather than inventing an original type name.

The positional arrangement is already preserved in the legacy
`CNewUISkillList`: origin (385,390) in the 640x480 reference space, 32x38
slots, alternating centered entries for the first 14, a four-entry left run
through entry 17, then a second row. The observable metadata strings
`NewUIMainFrameWindow-SkillListCenteredCount`,
`SkillListFirstRowCount`, `SkillListLeftRunStart`,
`SkillListSecondRowStart`, `SkillListSlotWidth`,
`SkillListSlotHeight`, and `SkillListIconOffset` expose the same
arrangement.

The reconstruction now loads the secondary skill-list document privately,
creates its icons with the recovered DOM sequence, scales legacy-reference
positions to the current viewport, and updates visibility/enabled/cooldown
state from `RmlMainFrameLayer::State::skillListSkills`.


## RmlPetFrameLayer reconstruction

The modern pet-frame asset exposes the exact document path and nine
presentation constants:

```text
Data/UI/PC/Character/pet_frame.rml
RmlPetFrameLayer-DragWidth
RmlPetFrameLayer-DragHeight
RmlPetFrameLayer-MemberHeight
RmlPetFrameLayer-ReferenceWidth
RmlPetFrameLayer-ReferenceHeight
RmlPetFrameLayer-MinimizeX
RmlPetFrameLayer-MinimizeY
RmlPetFrameLayer-MinimizeWidth
RmlPetFrameLayer-MinimizeHeight
```

The observable DOM contract contains `pet-frame`, `pet-dragbar`,
`pet-minimize`, and five fixed member rows
`pet-member-0..4` with matching `pet-name-N` and `pet-hp-N` elements.
Each member row also contains a second yellow HP bar, retained in the
reconstruction as an optional trailing/damage presentation ratio.

`RmlPetFrameLayer` is implemented as presentation-only state with up to five
members, main/yellow HP ratios, minimized state, movable positioning and a
one-shot `ToggleMinimized` action. The control does not claim ownership of
pet/game state and does not mutate any legacy pet subsystem.

Because the recovered design set contains no fixed initial X/Y pair, the
reconstruction preserves the movable panel's current position by default.
External code may request a position explicitly through the state snapshot.


## RmlPetInfoPanel reconstruction

The observable modern pet-information contract publishes the exact document
path and eleven design inputs:

```text
Data/UI/PC/Character/pet_info.rml
RmlPetInfoPanel-PanelWidth
RmlPetInfoPanel-PanelHeight
RmlPetInfoPanel-ReferenceWidth
RmlPetInfoPanel-ReferenceHeight
RmlPetInfoPanel-GfxStageWidth
RmlPetInfoPanel-InitialX
RmlPetInfoPanel-InitialY
RmlPetInfoPanel-TabX
RmlPetInfoPanel-TabY
RmlPetInfoPanel-TabWidth
RmlPetInfoPanel-TabHeight
```

The document contract exposes two tabs, five label/value rows, one progress
bar, a missing-pet message state, leadership/command fields, four visible
command-skill rows, one scrollbar, drag handle and close control.

The legacy `CNewUIPetInfoWindow` remains the source of game-side rules. Its
tab update calls `CalcDamage()`; rendering chooses Dark Horse or Dark Raven
data from equipped `PET_INFO`; and close remains a legacy interface action.
The modern reconstruction therefore exposes only two intents:
`SelectTab(index)` and `Close`.

`RmlPetInfoPanel` accepts presentation-neutral values, keeps four visible
skill rows backed by an arbitrary retained skill vector, reuses the recovered
`RmlMuScrollBar`, binds the movable panel, and applies the recovered authored
327x639 size plus initial 339,55 position. Skill rows carry a presentation
`data-skill-id` attribute but do not trigger pet commands themselves.


## RmlCommandWindowPanel reconstruction

The observable modern command-window asset exposes the exact document path,
nine presentation inputs, and eleven command controls:

```text
Data/UI/PC/Command/command_window.rml
RmlCommandWindowPanel-PanelWidth
RmlCommandWindowPanel-PanelHeight
RmlCommandWindowPanel-ReferenceWidth
RmlCommandWindowPanel-ReferenceHeight
RmlCommandWindowPanel-ButtonX
RmlCommandWindowPanel-ButtonY
RmlCommandWindowPanel-ButtonStep
RmlCommandWindowPanel-ButtonWidth
RmlCommandWindowPanel-ButtonHeight

command-button-0 .. command-button-10
command-label-0 .. command-label-10
```

The legacy `CNewUICommandWindow` still owns twelve internal command button
slots and all command execution/network rules (trade, purchase, party,
whisper, guild relationships, friend, follow, duel, and related permission
checks). The modern document visibly publishes only eleven button IDs, so the
reconstruction deliberately models **11 modern commands** and does not invent
a twelfth DOM control.

`RmlCommandWindowPanel` owns the presentation document, eleven button
wrappers, drag controller and close button. Its state supplies labels,
visible/enabled status and optional selected command. User interaction is
returned as one-shot `SelectCommand(index)` or `Close` intent; no network
request or legacy command is executed by the modern panel.


## RmlTooltipLayer reconstruction

The Debug executable exposes the exact tooltip document path
`Data/UI/PC/Common/tooltip.rml` and design keys:

```text
RmlTooltipLayer-TooltipCapacity
RmlTooltipLayer-MinimumContentSize
```

The bind body at `0x140832470` loads the document and loops from zero to the
configured tooltip capacity. It formats each id with the exact string
`tooltip-%d`, resolves the element, stores it in a retained array/vector, and
marks the layer bound only after every slot resolves. The observable asset
capacity is 8.

The presentation body recovers these exact property/class contracts:

- `min-width` formatted with `%fpx`;
- `text-align`: state 0 -> `left`, state 2 -> `right`, otherwise
  `center`;
- boolean CSS class `framed`;
- tooltip position through `left` and `top`, each formatted `%fpx`.

Line RML generation is also directly recoverable. Every line starts with
`<div class="tooltip-line`. A line whose first character is newline, or
whose content is exactly one space, receives class `blank` and no text body.
Otherwise, the first style enum maps values 1..9 to:

```text
blue
gray
red
yellow
green
purple
red-purple
violet
orange
```

A second enum maps values 1..4 to:

```text
dark-red
dark-blue
dark-yellow
green-blue
```

A boolean appends class `bold`. Nonblank text is encoded before insertion,
and every generated row closes with `</div>`.

The independently authored `RmlTooltipLayer` mirrors those observable
contracts with eight retained tooltip slots, presentation-neutral line/style
structures, exact class mappings, encoded text, frame/alignment/minimum-width
state, and left/top positioning.


## RmlFriendPanel reconstruction

The public MuClient friend document exposes a presentation contract with four
independent windows inside `Data/UI/PC/Friend/friend.rml`: the main friend
window, chat room window, write-letter window, and read-letter window. The
recovered design metadata is:

```text
RmlFriendPanel-PanelMainWidth = 540
RmlFriendPanel-PanelMainHeight = 381
RmlFriendPanel-PanelChatWidth = 553
RmlFriendPanel-PanelChatCollapsedWidth = 425
RmlFriendPanel-PanelChatHeight = 282
RmlFriendPanel-PanelWriteWidth = 429
RmlFriendPanel-PanelWriteHeight = 282
RmlFriendPanel-PanelReadWidth = 425
RmlFriendPanel-PanelReadHeight = 278
RmlFriendPanel-PanelMainVisibleRows = 5
RmlFriendPanel-PanelVisibleRows = 8
RmlFriendPanel-InitialX = 50
RmlFriendPanel-InitialY = 50
RmlFriendPanel-ChatVisibleLines = 11
```

The legacy MuMain side confirms that `CNewUIFriendWindow` is only an adapter
around `CUIWindowMgr`, with separate `CFriendList`, `CLetterList`, main
friend tabs, chat windows and letter read/write windows. Therefore the modern
reconstruction deliberately keeps protocol/window-manager ownership outside the
RmlUi presentation.

`RmlFriendPanel` now provides four independent visible states rather than a
single exclusive mode, matching the legacy manager's ability to keep the main
window and secondary chat/mail windows alive simultaneously. It binds the
observable DOM IDs, three tabs, eight list slots, four main actions, list
scrollbar, chat/invite controls, write/read forms, and movable panel behavior.

All user interaction is surfaced as a one-shot semantic `Action`; no network
packet, `CUIWindowMgr` command number, or private callback is invented.


## RmlMessageBoxPanel reconstruction

The observable `Data/UI/PC/Common/message_box.rml` contract defines one
reusable document for normal and S16-caution message boxes. Twenty design
values are recovered, including normal/caution dimensions, one/two-button
positions, and input limits of 20 password characters, 8 numeric characters,
and 10 text characters.

The legacy `CNewUIMessageBoxMng` confirms that message boxes are presentation
objects which emit semantic events (`COMMON_OK`, `COMMON_CANCEL`, Return,
Escape, and user-defined events) back to layout callbacks. The modern
reconstruction preserves that boundary instead of embedding game-side callback
logic.

`RmlMessageBoxPanel` now supports:

- normal and caution presentation;
- one-button and two-button layouts;
- message-only or input-bearing variants;
- text, numeric and password input contracts;
- the recovered max-length values;
- enabled/disabled OK and Cancel buttons;
- configurable Return/Escape behavior;
- centered viewport layout;
- one-shot actions carrying the current input value.

The message content is authored into DOM text nodes rather than interpreted as
RML, so game/server text cannot inject markup into the modern document.


## RmlPartyFrameLayer reconstruction

The observable party document defines five fixed member slots and a compact
drag/minimize header. The recovered presentation keys include the 640x480
reference space, 150x45 member rows, HP/MP gauge geometry, leader crown,
leave-button geometry, and initial Y=38.

The legacy `CNewUIPartyListWindow` confirms the behavioral ownership split:
party membership lives in the global `Party[]` model, the legacy window only
renders/selects it, and the leave control is available when the local user is
party leader or the row is the local user. Member HP is represented in ten
steps in the legacy mini-list.

The modern `RmlPartyFrameLayer` keeps those authority decisions outside the
presentation. Each member state supplies normalized HP/MP, leader,
out-of-sight/selection state, and an explicit `canLeave` flag. Interaction is
returned as semantic one-shot requests: minimize, select member, or leave
member. No party packet or global `Party[]` mutation is performed by RmlUi.


## RmlSystemMenuPanel reconstruction

The system-menu document exposes width 306, full/login heights, a 47.95-pixel
compact offset, three authored slot positions, divider/close positions, and
five semantic controls: exit, server, character, option, and close.

The existing `CNewUIWindowMenu` in MuMain is a separate compact window menu
and is not treated as the owner of this document. The reconstruction therefore
does not conflate its indexed menu commands with the RmlUi system menu.

`RmlSystemMenuPanel` treats the first four actions as independently
visible/enabled. Visible primary controls are packed at the recovered 47.95
vertical cadence; two-or-fewer primary actions use the recovered compact
height/offset. All interaction is surfaced as a semantic action enum. Escape
maps to Close, but logout/server/character/option operations are intentionally
left to the game-side owner.


## ServerMessage and StoreLabel layers

The remaining passive-overlay documents are now represented as presentation
layers rather than neutral skeletons.

`RmlServerMessageLayer` consumes the six recovered `ServerMessage-*`
design values (400x200 panel, left/top reference geometry) and renders server
text as a safe text node with no game-side callback.

`RmlStoreLabelLayer` consumes the recovered store-label dimensions,
world-raise, highlight period, and margin values. The caller supplies projected
screen anchors and value-only presentation state for player id, guild/title,
private store text, chat bubble, reputation, guild relation, gens rank and
castle mark. The RmlUi layer builds transient DOM labels and does not inspect
or own character objects.


## RmlMoveCommandPanel reconstruction

The move-command document exposes a fixed 12-row primary list, 5-row favorites
area, one vertical scrollbar, a show-map button, and close button. Its recovered
presentation contract includes the 376.998901 x 532.997894 panel size, row
height 20, main/favorite list anchors, checkbox/favorite column geometry,
scrollbar geometry, root Y=50, and 640x480 reference space.

The existing `CNewUIMoveCommandWindow` confirms that the authoritative data
already lives in `CMoveCommandData::MOVEINFODATA`: map index/name, required
level/max level/zen, `_bCanMove`, `_bStrife`, and selection state. Its
legacy click handler performs the actual warp request only after game-side
eligibility and cross-server checks.

The modern `RmlMoveCommandPanel` therefore consumes value-only destination
state and emits semantic requests. It does not send a warp packet. The primary
list scrolls over arbitrary destination counts through the reconstructed
`RmlMuScrollBar`, while the separately supplied favorite list uses five
authored slots. Favorite-cell clicks stop propagation and emit
`ToggleFavorite(mapIndex)`; row selection, show-map and close are separate
one-shot actions.


## RmlMasterTreePanel reconstruction

The public master-tree document contains **108 authored skill slots**, each with
a matching `mcSkillSlotN-btnBase`, plus `btnExp`, `btnClose`,
`btnDrag`, four top labels and three category labels. Recovered design
metadata:

```text
Panel-Size = 925.9 711.9
Skill-Columns = 25
Legacy-ReferenceSize = 640 480
Tooltip-BottomThreshold = 300
```

The existing `CNewUIMasterLevel` supplies stronger game-side semantics:

- three categories;
- category origins (11,55), (221,55), (431,55);
- column = `(Index - 1) % 4`;
- column step = 49;
- Y = categoryY + (rank - 1) * 41;
- two prerequisite skills;
- a parent prerequisite requires level >= 10;
- rank N requires sufficient points in rank N-1;
- required master points, max level, equipment/base-skill checks remain
  authoritative before upgrade confirmation.

The RmlUi reconstruction deliberately does not duplicate those eligibility
rules. `RmlMasterTreePanel::SkillSlot` receives the already evaluated
visible/enabled state, explicit modern slotId (1..108), skill id, group/rank/
column/direction and level. Its fallback placement is derived from the legacy
category/step geometry and scaled into the recovered 925.9x711.9 panel.
Clicking an enabled slot emits only `UpgradeSkill(skillId)`; the existing
game-side master-skill system remains responsible for confirmation and packet
submission.


## RmlOptionPanel reconstruction

The modern option document exposes a 390x580 panel and controls for automatic
attack, whisper sound, name display, sound/music levels 0..10, slide help,
effect level 0..4, render-all-effects, font, language, resolution, windowed
mode, and close.

The existing `CNewUIOptionWindow` confirms that most option changes are
applied immediately rather than staged behind an Apply button. In particular,
sound/music write live audio state and `GameConfig`; resolution/window mode
flow through `MuApplyWindowResolution`; font changes reinitialize fonts; and
language changes update the active localization.

`RmlOptionPanel` therefore emits one typed action per user change instead of
owning those side effects. Checkbox/radio controls carry boolean or integer
values; font/language/resolution selects emit their selected index; Escape and
the Close button emit Close. The game-side owner remains responsible for
persisting config and applying SDL/audio/localization/font changes.


## Shared Common RmlUi contracts

The neutral `Common/common.rml` and `Common/scrollbar.rml` skeletons have
been replaced with their observable design contracts. The common document
carries the global overflow-marquee values (0.6 second pause, 30 pixels/sec),
while the scrollbar document exposes the recovered 22x252.998 reference
geometry, track/button/thumb dimensions and minimum thumb height.

A local independently-authored `Common/common.rcss` now supplies the common
behavioral classes used throughout the reconstructed panels: buttons,
checkbox rows, meter cells, scrollbars, selects, text inputs, overflow labels,
window/divider bases and text areas. It intentionally does not copy the
upstream generated skin/decorator catalog.


## Inventory / expanded inventory presentation reconstruction

The x64 Debug executable preserves the exact document paths
`Data/UI/PC/Inventory/inventory.rml` and
`Data/UI/PC/Inventory/inventory_extension.rml` together with the complete
design-key contract below:

```text
Inventory-Size
Inventory-Grid
Inventory-Equipment0..11
Inventory-Reference
Inventory-InitialPosition
Inventory-Button0..5

Extension-Width
Extension-Heights
Extension-GridX
Extension-GridY
Extension-PitchX
Extension-PitchY
Extension-Reference
Extension-InitialPosition
```

The Debug string inventory also preserves the observable DOM/semantic names
`inventory`, `inventory-extension-`, equipment/grid terminology and the
expanded-inventory user-facing strings.

No RTTI name for a dedicated modern Inventory controller is present in the
current Debug evidence. Therefore the local type name
`UI::Modern::PC::Inventory::RmlInventoryPanel` is explicitly a semantic
reconstruction name, not a claim about the original private source identifier.

The reconstructed controller keeps the existing legacy inventory as source of
truth. It owns only RmlUi presentation state:

- lazy hosts for base and expanded inventory documents;
- recovered initial positions and dimensions;
- six action buttons;
- 64 base inventory slots;
- 13 named equipment/pentagram presentation slots;
- 128 expanded slots (32 per unlocked bag, up to four bags);
- drag controllers and one-shot primary/secondary slot requests.

Item ownership, validation, equip/use rules, repair logic, private-store rules
and network requests remain in the legacy inventory system until their exact
modern ownership/callback graph is recovered.


## Inventory design contract reconstruction

The x64 Debug string inventory exposes the complete design-key surface for the
main inventory document:

```text
Inventory-Size
Inventory-Reference
Inventory-InitialPosition
Inventory-Grid
Inventory-Equipment0 .. Inventory-Equipment11
Inventory-Button0 .. Inventory-Button5
```

It also preserves the dynamic slot prefix `isSlot` and named equipment ids:

```text
slot_weapon_right
slot_weapon_left
slot_helm
slot_armor
slot_pants
slot_gloves
slot_boots
slot_wing
slot_helper
slot_amulet
slot_ring_right
slot_ring_left
```

The reconstructed `RmlInventoryPanel` consumes those authored metrics,
creates 64 `isSlotN` inventory slots at runtime, binds twelve equipment slots
with `RmlMuSlot`, and exposes semantic one-shot actions for close, repair,
private store, extension bag, socket/set option, and primary/secondary slot
interaction.

The legacy `CNewUIMyInventory` remains authoritative for item ownership,
drag/drop, repair eligibility, networking and equipment validation. This
presentation controller intentionally does not send item packets or mutate
`CNewUIInventoryCtrl` state.


## Private Store presentation reconstruction

The x64 Debug executable preserves the exact Private Store design-key set:

```text
PrivateStore-DropAllowedColor
PrivateStore-DropBannedColor
PrivateStore-seller-GridRect
PrivateStore-buyer-GridRect
PrivateStore-Width
PrivateStore-Height
PrivateStore-GridColumns
PrivateStore-GridRows
PrivateStore-InitialX
PrivateStore-InitialY
PrivateStore-ReferenceWidth
PrivateStore-ReferenceHeight
```

It also preserves the document path
`Data/UI/PC/Inventory/private_store.rml`, fixed DOM IDs
`private-store`, `private-store-name`, `private-store-buyer-name`,
`private-store-open`, `private-store-close-shop`,
`private-store-close`, `private-store-drag`, and the exact dynamic slot
prefix `private-store-slot-`. The strings `private-store-seller-` and
`private-store-buyer-` additionally confirm distinct seller/buyer
presentation paths.

No dedicated RmlUi private-store class RTTI name is currently present in the
Debug evidence. The local `RmlPrivateStorePanel` name is therefore semantic,
while document/ID/design contracts are evidence-derived.

The reconstruction materializes an 8x4 (32-slot) grid, seller/buyer mode,
25-character store-name input, drag/close/open/close-shop controls, and
Debug-matched allowed/banned drop colors. All actions and slot clicks are
one-shot requests; pricing, item legality, purchase/open requests and network
ownership remain in the legacy private-store code.


## Inventory extension design contract

The Debug executable preserves these authored keys:

```text
Extension-Width
Extension-Heights
Extension-GridX
Extension-GridY
Extension-PitchX
Extension-PitchY
Extension-Reference
Extension-InitialPosition
```

The existing legacy `CNewUIInventoryExtension` confirms the semantic
organization: four extension inventories are created up front, each is an
8-column x 4-row `CNewUIInventoryCtrl` (32 cells), and only the first
`CharacterAttribute->InventoryExtensions` groups participate in update and
render. The controller therefore represents 4 x 32 = 128 slots and keeps
unopened groups presentation-disabled.

`RmlInventoryExtensionPanel` reconstructs that presentation boundary while
leaving item movement, ownership, network requests, and empty-slot searches in
the legacy extension controls.


## Private Store design contract

The x64 Debug executable preserves the full private-store design surface:

```text
PrivateStore-Width
PrivateStore-Height
PrivateStore-InitialX
PrivateStore-InitialY
PrivateStore-ReferenceWidth
PrivateStore-ReferenceHeight
PrivateStore-seller-GridRect
PrivateStore-buyer-GridRect
PrivateStore-GridColumns
PrivateStore-GridRows
PrivateStore-DropAllowedColor
PrivateStore-DropBannedColor
```

It also exposes the exact DOM identifiers `private-store`,
`private-store-drag`, title/name/open/close-shop controls, and the dynamic
slot prefix `private-store-slot-`.

The reconstructed `RmlPrivateStorePanel` supports seller/buyer presentation,
32 slots (8x4), the authored drop-feedback colors, editable seller title
(maxlength 25), open/close-shop/close-window intents, and primary/secondary
slot requests. Legacy `CNewUIMyShopInventory` and
`CNewUIPurchaseShopInventory` remain responsible for price dialogs, item
eligibility, purchase/open/close packets and inventory ownership.


## MuHelper presentation reconstruction

The x64 Debug executable preserves the exact document path
`Data/UI/PC/MuHelper/mu_helper.rml` and design keys:

```text
MuHelper-Width
MuHelper-Height
MuHelper-SidePanelWidth
```

The public observable document contract additionally carries
`MuHelper-ReferenceWidth` and `MuHelper-ReferenceHeight`, and exposes the
stable DOM ids for tabs, hunting/obtaining ranges, checkbox options, skill
slots, extra-item list, reset/save/close and advanced-settings launchers.

No dedicated modern MuHelper class RTTI name is currently preserved in the
Debug image. The local `RmlMuHelperPanel` name is therefore semantic.
Unlike a disconnected presentation model, however, its state type is the
existing legacy/domain `MUHelper::ConfigData`.

The first materialized layer maps the unambiguous domain fields directly:
hunting/obtaining range, fallback basic attack, long-range counter, return
position/time, potion/combo/raven/support/heal/drain/buff options, repair and
item-pick filters, extra item names, friend/guild acceptance and self-defense.
It binds six assigned-skill slots and ten available-skill presentation slots,
plus a five-row scrolling extra-item list.

Save/reset/close and advanced option buttons are exposed as one-shot actions.
The panel intentionally does not send `SendMuHelperSaveDataRequest` or status
packets itself; network/domain ownership remains with the existing
`MUHelper::CMuHelper` and legacy UI until the higher-level ownership bridge is
validated.


## Login / Login Bottom presentation reconstruction

The x64 Debug executable preserves both exact document paths:

```text
Data/UI/PC/Login/login.rml
Data/UI/PC/Login/login_bottom.rml
```

and these design keys:

```text
Login-Width
Login-Height
Login-AnchorX
Login-AnchorY
Login-Szene

LoginBottom-Columns
LoginBottom-Width
LoginBottom-Height
LoginBottom-HorizontalMargin
LoginBottom-BottomMargin
LoginBottom-BaselineY
```

The Debug strings also preserve the stable DOM ids `login-panel`,
`account-input`, `password-input`, `account-label`,
`password-label`, and the `login-` / `login-buttons-` prefixes. The
public observable asset confirms the OK/Cancel plus Menu/Credit control
contract.

No dedicated modern login controller RTTI name is currently recovered, so
`RmlLoginPanel` is a semantic local name. It hosts the login and bottom
documents, applies the recovered anchor and bottom-control layout, and exposes
login credentials and cancel/menu/credit actions as one-shot requests.

The panel does not call `SendLoginLongPassword` or
`SendLoginShortPassword`. Password text is cleared on hide, cancel and after
the login request is consumed, keeping network/authentication ownership in the
existing login scene.


## MU Helper presentation contract

The x64 Debug executable preserves the authored keys
`MuHelper-Width`, `MuHelper-Height`, and
`MuHelper-SidePanelWidth`, together with the document and control identifiers
including `mu-helper`, `helper-drag`, `helper-close`,
`helper-reset`, and `helper-save`.

The existing legacy `CNewUIMuHelper` confirms three primary tabs, hunting and
obtaining range controls, six assigned skill slots, a selectable skill list,
extra-item list management, reset/save/close actions and class-dependent
advanced configuration pages. `MUHelper::ConfigData` remains the
authoritative serialization/network domain.

The reconstructed `RmlMuHelperPanel` independently authors the presentation
and exposes semantic actions for the primary tabs, ranges, major boolean
options, skill-slot selection, extra-item add/delete and advanced configuration
entry points. It deliberately does not serialize packets or replace the
existing `ConfigDataSerDe` path.


## Storage presentation contract

The x64 Debug executable preserves the basenames
`storage.rml`, `storage_extension.rml`,
`storage_keypad.rml`, and `storage_password.rml`. The public observable
storage document exposes the generic authored panel keys
`Panel-Size`, `Panel-Grids`, `Panel-Reference`, and
`Panel-Initial`, plus stable DOM identifiers for title/drag/close, Zen
controls, extension control, period-storage fields, and the exact dynamic
`isSlotN` slot convention.

The existing legacy `CNewUIStorageInventory` confirms the main vault grid as
8 columns x 15 rows (120 slots), and remains authoritative for vault lock
status, password validation, deposit/withdraw dialogs, item auto-move and all
network requests.

The reconstructed `RmlStoragePanel` therefore owns only presentation state:
120 RmlUi slots, stored-Zen/fee/period text, locked/unlocked styling and
one-shot close/deposit/withdraw/lock/extend/Zen-info/slot actions. It is owned
by the central `RmlPcUiHost` but remains unloaded by default so the legacy
storage stays the active surface until explicitly bridged.


### Expanded storage presentation

The legacy `CNewUIStorageInventoryExt` creates a second vault
`CNewUIInventoryCtrl` with the same 8x15 geometry and an index offset of
`MAX_SHOP_INVENTORY`. The public observable `storage_extension.rml`
reuses the same `Panel-Size`, `Panel-Grids`, and `Panel-Reference`
contract, but starts at `Panel-Initial = 360 40`.

`RmlStorageExtensionPanel` mirrors the second 120-slot presentation and emits
only close/exit/primary/secondary-slot intents. The global vault index offset,
auto-move and request packets remain in `CNewUIStorageInventoryExt` and the
main storage controller.


## Main-frame legacy state bridge

The reconstructed PC host now includes a one-way
`RmlMainFrameLegacyBridge`. It does not load or show the modern HUD. It is
invoked only when `RmlMainFrameLayer` has already been explicitly loaded,
preserving the legacy frame as the default visual surface.

The bridge deliberately reads the same public state used by
`CNewUIMainFrameWindow`:

- `CNewUIMainFrameWindow::IsVisible()`;
- `CharacterAttribute` Life/Mana/Shield/SkillMana values;
- master maxima from `Master_Level_Data`;
- the exact normal/master experience lower-bound formulas from
  `CNewUIMainFrameWindow::RenderExperience()`;
- poison state through `g_isCharacterBuff(..., eDeBuff_Poison)`;
- current skill from `Hero->CurrentSkill`;
- hotkeys through `CNewUIMainFrameWindow::GetSkillHotKey()`;
- skill-index lookup and hotkey-page state through the public
  `CNewUISkillList` API;
- cooldown ratio from `CharacterAttribute->SkillDelay` and
  `SkillAttribute[].Delay`.

A source-level detail was confirmed while building the bridge:
`CNewUISkillList::IsSkillListUp()` returns the private
`m_bHotKeySkillListUp`, selecting hotkey page 1..5 versus 6..0. It does
**not** expose the separate `m_bSkillList` full chooser visibility. The
bridge therefore synchronizes the correct hotkey page and builds the filtered
skill collection, but intentionally keeps the reconstructed full skill chooser
hidden instead of inferring a private boolean from mouse state.

This remains a read-only bridge. Modern main-frame button actions are not yet
forwarded into legacy window/game commands.


### Storage keypad/password security flow

The Debug basenames `storage_keypad.rml` and `storage_password.rml` map
cleanly to the existing legacy `CNewUIKeyPadMsgBox` and password-input
layouts. The legacy keypad defaults to a 4-digit input limit, maintains ten
button positions with a randomized `m_iKeyPadMapping[]`, supports delete,
and exposes OK/Cancel callbacks. Storage lock/unlock layouts then send the
result through `SendUnlockVault`, `SendSetVaultPin`, or
`SendRemoveVaultPin` depending on the higher-level flow.

`RmlStorageSecurityPanel` reconstructs only the modal presentation and input
contract. Its state accepts the ten-position digit mapping, applies the 4-digit
limit, masks the displayed PIN, supports backspace and returns one-shot
SubmitPin/SubmitPassword/Cancel intents. It never validates the account
password and never sends vault packets; those responsibilities remain in the
legacy message-box/network path.


## MuHelper legacy state bridge

`RmlMuHelperLegacyBridge` is now the read-only adapter between the existing
helper domain and the modern presentation. It is executed only when the modern
MuHelper panel has already been explicitly loaded.

The bridge reads:

- visibility through `CNewUISystem::IsVisible(INTERFACE_MUHELPER)`;
- `MUHelper::g_MuHelper.GetConfig()`;
- hunting and obtaining ranges;
- return-position/time, potion, combo, Dark Raven, support/heal/drain/buff;
- repair and all item-pick filters;
- auto-accept friend/guild, self-defense and fallback-basic-attack local bits;
- three configured attack skills plus three configured buff skills;
- the extra-item `std::set<std::wstring>`, converted to UTF-8.

The legacy UI does not expose its current main tab or which attack/buff skill
chooser subwindow is active. The bridge therefore does not guess those private
presentation fields and leaves the available-skill chooser empty until that
state is recovered.

Save/start/stop/network ownership remains in `MUHelper::CMuHelper` and the
legacy UI. This bridge performs no packet transmission and no legacy mutation.


## Trade presentation contract

The Debug executable preserves the basenames `trade.rml` and
`trade_confirm.rml`. Their observable authored contract uses
`Panel-Size`, `Panel-Grids`, `Panel-Reference`, and
`Panel-Initial`, with two 8x4 grid origins in `Panel-Grids`.

The legacy `CNewUITrade` confirms two 32-slot inventory controls: the other
player at the upper grid and the local player at the lower grid. It also owns
other-player identity/guild/level, both Zen values, local/remote confirmation,
the anti-spam confirmation wait, trade-alert state, warning-marked changed
items, and all trade/network requests.

`RmlTradePanel` hosts both trade documents, reconstructs the two 32-slot
surfaces and confirm overlays, and returns one-shot close, Zen-input,
confirm-toggle, local-slot and remote-inspect intents. It does not send
`SendTradeCancel`, `SendSetTradeMoney`,
`SendTradeButtonStateChange`, or equipment-item requests.


## Inventory legacy state bridge

`RmlInventoryLegacyBridge` now projects public legacy inventory state into an
explicitly loaded `RmlInventoryPanel`, without transferring or retaining any
`ITEM*`.

The bridge reads:

- visibility through `CNewUISystem::IsVisible(INTERFACE_INVENTORY)`;
- repair availability and active repair mode from
  `CNewUIMyInventory`;
- personal-store availability (legacy level >= 6 rule) and current shop-window
  visibility;
- all 64 base cells through
  `CNewUIInventoryCtrl::FindItem(column,row)`;
- the exact pointed-square index through
  `GetPointedSquareIndex()/GetIndex()`;
- up to 12 reconstructed equipment presentation slots from
  `CharacterMachine->Equipment[]` and
  `CNewUIMyInventory::GetPointedItemIndex()`.

Only occupied/selected/enabled presentation state is copied. `RmlMuSlot`'s
`iconFrame` is intentionally left neutral because that field selects a CSS
frame class and is not an item texture/type identifier. Actual item 3D/sprite
rendering remains a separate asset/render bridge.

No item pointer ownership, movement, repair, drop, equip, personal-store or
network logic is moved out of the legacy inventory.


## Expanded Inventory legacy state bridge

`RmlInventoryExtensionLegacyBridge` mirrors the public extension state only
when the modern extension panel is already loaded.

The legacy `CNewUIInventoryExtension::Create()` confirms four independently
created 8x4 controls, each with
`MAX_INVENTORY_EXT_ONE` slots and global index offsets beginning at
`MAX_MY_INVENTORY_INDEX`. The bridge therefore maps:

- visible state from `INTERFACE_INVENTORY_EXT`;
- opened bag count from `CharacterAttribute->InventoryExtensions`, clamped
  to the reconstructed four-bag contract;
- 128 presentation slots, 32 per bag;
- occupancy by calling the public
  `CNewUIInventoryExtension::FindItem(globalIndex)`.

Pointed-square/drag ownership is private to the individual extension controls,
so the bridge does not guess a selected slot. No `ITEM*` is retained and no
movement/network behavior is duplicated.


## Private Store legacy state bridge

`RmlPrivateStoreLegacyBridge` unifies the two proven legacy personal-store
windows into the reconstructed seller/buyer presentation, but only after that
modern panel has been explicitly loaded.

Seller mode reads:

- `INTERFACE_MYSHOP_INVENTORY` visibility;
- `CNewUIMyShopInventory::IsEnablePersonalShop()`;
- current title through `GetTitle()`;
- its public inventory control and pointed-square index.

Buyer mode reads:

- `INTERFACE_PURCHASESHOP_INVENTORY` visibility;
- `CNewUIPurchaseShopInventory::GetTitleText()`;
- its locked inventory control and pointed-square index.

Both modes project the same 8x4 / 32-slot occupancy map through public
`FindItem(column,row)` and `GetIndex(column,row)`. No item or price pointer
is retained.

The reconstructed drop-state colors are intentionally left neutral here. The
legacy seller calculates allowed/banned drop state from the **currently picked
item** during drag-and-drop; reproducing that transient feedback belongs in the
future picked-item/action bridge rather than the steady-state snapshot.


## RmlServerSelectPanel reconstruction

The public MuClient PC tree exposes the document
`Data/UI/PC/ServerSelect/server_select.rml` with the stable DOM ids
`server-select`, `title`, `group-list`, `server-list`,
`description`, and `advisory`. The local RML/RCSS is independently
authored around that observable contract.

The existing MuMain `CServerListManager` is kept authoritative. It exposes
the already-decoded server groups and server entries, including group names and
descriptions, `m_iConnectIndex`, per-server index, load percentage, and the
legacy `m_byNonPvP` classification:

```text
0 = PVP
1 = Non-PVP
2 = Gold PVP
3 = Gold
```

The bridge copies this state into a presentation-neutral panel model without
retaining `CServerGroup*` or `CServerInfo*`. A selected modern row returns
a one-shot selection payload containing the group key, connection index,
server index, classification and displayed name. It deliberately does not call
`SetSelectServerInfo` or start the connection: that side effect remains at
the legacy scene/network boundary until the exact callback ownership is
recovered.


## RmlCharacterCreatePanel reconstruction

The public MuClient document
`Data/UI/PC/Character/character_create.rml` exposes the seven class button
ids `btnClass0..btnClass6`, `btnOK`, `btnCancel`, the input
`tiCharacterName`, the five stat groups, and preview/description ids. Its
observable metadata includes the authored panel size `857.517712 x 625`,
reference `640 x 480`, preview camera tuple, 25 effect frames and 30 FPS.

The legacy `CCharMakeWin` remains authoritative for all game-side behavior.
Its source confirms seven classes, class-specific base stats, Dark Lord's fifth
leadership stat, 10-character input limit, minimum-name validation, forbidden
name checks, and the eventual `SendCreateCharacter` packet. The reconstruction
does not duplicate that packet path.

A new read-only `CCharMakeSnapshot` projects only visibility, selected class,
class availability/localized labels, stat labels/values and description.
`RmlCharacterCreateLegacyBridge` converts that snapshot to UTF-8 and feeds
the modern panel. The modern panel emits one-shot `SelectClass`,
`Create(name,class)` and `Cancel` intents; applying those intents to the
legacy window/network remains a separate callback integration step.


## RmlBuffListLayer reconstruction

The public MuClient HUD exposes `Data/UI/PC/HUD/buff_list.rml` with
containers `_BuffList` and `_DeBuffList` and observable metadata:
authored size 366x36, 26x36 cells, eight-column layout, small-stage threshold
700 with scale 0.83, and the published supported buff-state icon inventory.

The existing `CNewUIBuffWindow::BuffSort` is retained as the authoritative
ordering/filtering path. It normalizes upgraded variants to their base buff,
keeps the highest tier, excludes non-rendered transient states, and places
buffs before debuffs. A new read-only `BuildSnapshot` simply projects that
already-sorted result with `g_CharacterBuffCount`,
`g_IsBuffClass`, and `g_BuffStringTime`.

The modern layer dynamically builds retained icons and emits clicked buff-state
ids only. Cancellation policy (for example Infinity Arrow / Swell of Magic
Power confirmation) remains in the legacy game UI and is not duplicated.
