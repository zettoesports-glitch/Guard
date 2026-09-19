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
  alternate/bone-scale transform path (bit 4), and terrain-light override
  (bit 5).
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
