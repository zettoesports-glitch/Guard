#pragma once

#include "client/render/LogicalRenderAssetTable.h"
#include "client/render/RenderTapeTypes.h"
#include "client/render/SessionRenderTape.h"
#include "Render/Renderer/MuRenderer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mu::pipeline
{

class LegacyRenderFacade
{
public:
    explicit LegacyRenderFacade(LogicalRenderAssetTable& assets) noexcept;

    [[nodiscard]] bool BeginPass(RenderTapePass pass, const SessionFogPassConstants& fog) noexcept;
    [[nodiscard]] bool EndPass() noexcept;
    [[nodiscard]] std::optional<SessionRenderTape> Finalize() noexcept;
    [[nodiscard]] bool IsRecording() const noexcept { return m_recording.IsRecording(); }

    [[nodiscard]] bool Begin(LegacyPrimitive primitive) noexcept;
    [[nodiscard]] bool End() noexcept;
    [[nodiscard]] bool Vertex3(float x, float y, float z) noexcept;
    [[nodiscard]] bool Color4(float r, float g, float b, float a) noexcept;
    [[nodiscard]] bool Normal3(float x, float y, float z) noexcept;
    [[nodiscard]] bool TexCoord2(float u, float v) noexcept;
    [[nodiscard]] bool EmitPrimitiveDraw(LegacyPrimitive primitive, std::span<const RenderTapeVertex> vertices) noexcept;

    // Integration helpers for already-modernized MuMain paths. These keep direct
    // IMuRenderer submissions tape-compatible while the original MuTwo logical
    // geometry asset layer is reconstructed.
    [[nodiscard]] bool SubmitTriangles(std::span<const mu::Vertex3D> vertices, std::uint32_t textureId) noexcept;
    [[nodiscard]] bool SubmitQuad3D(std::span<const mu::Vertex3D> vertices, std::uint32_t textureId) noexcept;
    [[nodiscard]] bool SubmitQuad2D(std::span<const mu::Vertex2D> vertices, std::uint32_t textureId) noexcept;
    [[nodiscard]] bool SubmitTextTriangles(std::span<const mu::Vertex2D> vertices,
                                           void* atlasTexture, void* sampler = nullptr) noexcept;
    [[nodiscard]] bool SubmitLines(std::span<const mu::Vertex3D> vertices, std::uint32_t textureId) noexcept;
    [[nodiscard]] bool SubmitSkinnedTriangles(std::span<const mu::SkinnedVertex3D> vertices,
                                              std::uint32_t textureId,
                                              const mu::SkinningParameters& parameters) noexcept;

    [[nodiscard]] bool SetClientArray(RenderClientArraySemantic semantic, std::span<const std::byte> bytes,
                                      std::uint32_t componentCount, RenderClientArrayScalarType scalarType,
                                      int stride, bool normalized) noexcept;
    void EnableClientArray(RenderClientArraySemantic semantic) noexcept;
    void DisableClientArray(RenderClientArraySemantic semantic) noexcept;
    [[nodiscard]] bool DrawArrays(LegacyPrimitive primitive, int first, int count) noexcept;

    [[nodiscard]] bool MatrixMode(LegacyMatrixMode mode) noexcept;
    [[nodiscard]] bool LoadIdentity() noexcept;
    [[nodiscard]] bool LoadMatrix(const std::array<float, 16>& matrix) noexcept;
    [[nodiscard]] bool MultMatrix(const std::array<float, 16>& matrix) noexcept;
    [[nodiscard]] bool Translate(float x, float y, float z) noexcept;
    [[nodiscard]] bool Scale(float x, float y, float z) noexcept;
    [[nodiscard]] bool Rotate(float angle, float x, float y, float z) noexcept;
    [[nodiscard]] bool Ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane) noexcept;
    [[nodiscard]] bool Perspective(float fovY, float aspect, float nearPlane, float farPlane) noexcept;
    [[nodiscard]] bool PushMatrix() noexcept;
    [[nodiscard]] bool PopMatrix() noexcept;
    [[nodiscard]] bool ApplyMatrix(const std::array<float, 16>& matrix) noexcept;
    [[nodiscard]] bool PushAttrib() noexcept;
    [[nodiscard]] bool PopAttrib() noexcept;
    [[nodiscard]] bool PushClientAttrib() noexcept;
    [[nodiscard]] bool PopClientAttrib() noexcept;

    [[nodiscard]] bool SetOpaqueState() noexcept;
    [[nodiscard]] bool SetAlphaTestState(bool enabled) noexcept;
    [[nodiscard]] bool SetAdditiveState() noexcept;
    [[nodiscard]] bool SetTextureEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetDepthTestEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetDepthWriteEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetDepthFunc(RenderCompareFunction func) noexcept;
    [[nodiscard]] bool SetCullEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetCullFace(RenderCullFace face) noexcept;
    [[nodiscard]] bool SetFrontFace(RenderFrontFace face) noexcept;
    [[nodiscard]] bool SetBlendEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetBlendFunc(RenderBlendFactor source, RenderBlendFactor destination) noexcept;
    [[nodiscard]] bool SetAlphaTestEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetAlphaFunc(RenderCompareFunction func, float ref) noexcept;
    [[nodiscard]] bool SetFogEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetFogMode(RenderFogMode mode) noexcept;
    [[nodiscard]] bool SetFogColor(const std::array<float, 4>& color) noexcept;
    [[nodiscard]] bool SetFogRange(float start, float end) noexcept;
    [[nodiscard]] bool SetFogDensity(float density) noexcept;
    [[nodiscard]] bool SetLightingEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetColorMask(bool r, bool g, bool b, bool a) noexcept;
    [[nodiscard]] bool SetStencilEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetStencilFunc(RenderCompareFunction func, unsigned int ref, unsigned int mask) noexcept;
    [[nodiscard]] bool SetStencilOp(RenderStencilOperation fail, RenderStencilOperation depthFail,
                                    RenderStencilOperation pass) noexcept;
    [[nodiscard]] bool SetTextureEnvironment(RenderTextureEnvironment environment) noexcept;
    [[nodiscard]] bool SetShadeMode(RenderShadeMode mode) noexcept;
    [[nodiscard]] bool SetLineWidth(float width) noexcept;
    [[nodiscard]] bool SetPolygonMode(RenderCullFace face, RenderPolygonMode mode) noexcept;
    [[nodiscard]] bool SetViewport(RenderTapeRect rect) noexcept;
    [[nodiscard]] bool SetScissorEnable(bool enabled) noexcept;
    [[nodiscard]] bool SetScissor(RenderTapeRect rect) noexcept;
    [[nodiscard]] bool SetClearColor(const std::array<float, 4>& color) noexcept;
    [[nodiscard]] bool SetClearDepth(float depth) noexcept;
    [[nodiscard]] bool SetClearStencilValue(unsigned int value) noexcept;
    [[nodiscard]] bool Clear(bool color, bool depth, bool stencil) noexcept;
    [[nodiscard]] bool ClearStencil(unsigned int value) noexcept;

    [[nodiscard]] bool DefineTexture2D(LogicalRenderAssetRef ref, unsigned int width, unsigned int height,
                                       std::span<const std::byte> pixels, LegacyPixelFormat format,
                                       RenderAssetRetention retention, RenderSamplerIntent sampler) noexcept;
    void BindTexture(LogicalRenderAssetRef ref) noexcept;
    void BindTextureId(std::uint32_t textureId) noexcept;

private:
    struct ClientArray
    {
        std::vector<std::byte> bytes;
        std::uint32_t componentCount = 0;
        RenderClientArrayScalarType scalarType = RenderClientArrayScalarType::Float;
        int stride = 0;
        bool normalized = false;
        bool enabled = false;
    };

    [[nodiscard]] static std::uint32_t PackColor(const std::array<float, 4>& color) noexcept;
    [[nodiscard]] bool ReadArrayVertex(int index, RenderTapeVertex& vertex) const noexcept;
    [[nodiscard]] bool RefreshFog() noexcept;

    LogicalRenderAssetTable& m_assets;
    SessionRenderTapeRecording m_recording;
    RenderTapeState m_state{};
    LegacyPrimitive m_primitive = LegacyPrimitive::Triangles;
    bool m_insidePrimitive = false;
    std::vector<RenderTapeVertex> m_vertices;
    RenderTapeVertex m_current{};
    std::uint32_t m_boundTextureId = 0;

    ClientArray m_vertexArray;
    ClientArray m_normalArray;
    ClientArray m_colorArray;
    ClientArray m_texCoordArray;

    struct ClientAttribSnapshot
    {
        ClientArray vertex;
        ClientArray normal;
        ClientArray color;
        ClientArray texCoord;
    };

    std::vector<RenderTapeState> m_attribStack;
    std::vector<ClientAttribSnapshot> m_clientAttribStack;

    int m_fogMode = static_cast<int>(RenderFogMode::Linear);
    float m_fogStart = 0.0f;
    float m_fogEnd = 1.0f;
    float m_fogDensity = 1.0f;
    std::array<float, 4> m_fogColor{0.0f, 0.0f, 0.0f, 1.0f};
};

LegacyRenderFacade& GetLegacyRenderFacade();

} // namespace mu::pipeline
