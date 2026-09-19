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
ReserveIndexedTriangles(unsigned int,unsigned int)
SetClientArray(RenderClientArraySemantic, span<byte const>, unsigned int,
               RenderClientArrayScalarType, int, bool) noexcept
EnableClientArray(RenderClientArraySemantic) noexcept
DisableClientArray(RenderClientArraySemantic) noexcept
ReadArrayVertices(int,int,span<RenderTapeVertex>) noexcept
CountClassifiedArrayTriangles(LegacyPrimitive,int,int,unsigned int&,unsigned int&) noexcept
DrawArrays(LegacyPrimitive,int,int) noexcept
DrawGeometry(LogicalGeometryAssetLease const&,unsigned int,unsigned int) noexcept
BuildTrustedGeometryDraw(LogicalGeometryAssetLease const&,unsigned int,unsigned int,
                         TrustedGeometryDraw&) noexcept
AppendTrustedGeometryDrawBatch(span<TrustedGeometryDraw const>) noexcept
DrawTerrainInstances(LogicalGeometryAssetLease const&,unsigned int,unsigned int,
                     RenderTapeTerrainConstants const&) noexcept
DrawGrassGeometry(LogicalGeometryAssetLease const&,unsigned int,unsigned int,uint64) noexcept
BeginQuadInstanceRun() -> uint64
ReserveTrailSamples(unsigned int)
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
Sphere(float,unsigned int,unsigned int) noexcept
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
                           RenderSamplerIntent,array<array<float,2>,4> const&) noexcept
BindTexture(LogicalRenderAssetRef) noexcept
CopyTargetToLogicalTexture(SessionId,SessionGeneration,uint64,RenderTapeRect,
                           LogicalRenderAssetRef) noexcept
DownloadTargetRgba8(SessionId,SessionGeneration,uint64,uint64,RenderTapeRect,bool,uint64) noexcept
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
