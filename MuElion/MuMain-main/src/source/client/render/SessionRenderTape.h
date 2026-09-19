#pragma once

#include "client/render/RenderTapeTypes.h"
#include "Render/Renderer/MuRenderer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace mu::pipeline
{

enum class RenderTapePass : std::uint8_t
{
    World = 0,
    Effects = 1,
    Ui = 2,
    Overlay = 3,
};

struct SessionFogPassConstants
{
    bool enabled = false;
    RenderFogMode mode = RenderFogMode::Linear;
    float start = 0.0f;
    float end = 1.0f;
    float density = 1.0f;
    std::array<float, 4> color{0.0f, 0.0f, 0.0f, 1.0f};
};

struct RenderTapeState
{
    bool textureEnabled = true;
    bool depthTestEnabled = false;
    bool depthWriteEnabled = true;
    bool cullEnabled = false;
    bool blendEnabled = false;
    bool alphaTestEnabled = false;
    bool fogEnabled = false;
    bool stencilEnabled = false;
    bool scissorEnabled = false;
    bool lightingEnabled = false;

    RenderCompareFunction depthFunc = RenderCompareFunction::LessEqual;
    RenderCompareFunction alphaFunc = RenderCompareFunction::Greater;
    float alphaRef = 0.0f;
    RenderBlendFactor blendSource = RenderBlendFactor::SourceAlpha;
    RenderBlendFactor blendDestination = RenderBlendFactor::OneMinusSourceAlpha;
    RenderFrontFace frontFace = RenderFrontFace::CounterClockwise;
    RenderCullFace cullFace = RenderCullFace::Back;
    RenderShadeMode shadeMode = RenderShadeMode::Smooth;
    RenderPolygonMode polygonMode = RenderPolygonMode::Fill;
    RenderTextureEnvironment textureEnvironment = RenderTextureEnvironment::Modulate;
    float lineWidth = 1.0f;

    RenderCompareFunction stencilFunc = RenderCompareFunction::Always;
    unsigned int stencilRef = 0;
    unsigned int stencilMask = ~0u;
    RenderStencilOperation stencilFail = RenderStencilOperation::Keep;
    RenderStencilOperation stencilDepthFail = RenderStencilOperation::Keep;
    RenderStencilOperation stencilPass = RenderStencilOperation::Keep;

    bool colorMaskR = true;
    bool colorMaskG = true;
    bool colorMaskB = true;
    bool colorMaskA = true;

    RenderTapeRect viewport{};
    RenderTapeRect scissor{};

    int fogMode = static_cast<int>(RenderFogMode::Linear);
    float fogStart = 0.0f;
    float fogEnd = 1.0f;
    float fogDensity = 1.0f;
    std::array<float, 4> fogColor{0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 4> clearColor{0.0f, 0.0f, 0.0f, 0.0f};
    float clearDepth = 1.0f;
    unsigned int clearStencil = 0;

    std::array<float, 16> modelView{};
    std::array<float, 16> projection{};
};

struct RenderTapeDraw
{
    LegacyPrimitive primitive = LegacyPrimitive::Triangles;
    bool screenSpace2D = false;
    std::uint32_t textureId = 0;
    RenderTapeState state{};
    std::vector<RenderTapeVertex> vertices;
};

struct RenderTapeSkinningSnapshot
{
    std::vector<float> boneMatrices;
    std::uint32_t paletteVersion = 0;
    std::array<float, 3> bodyOrigin{};
    float bodyScale = 1.0f;
    float boneScale = 1.0f;
    float restPoseScale = 0.0f;
    std::array<float, 3> lightDirection{};
    std::array<float, 2> textureCoordinateOffset{};
    float chromeWave = 0.0f;
    float chromeWave2 = 0.0f;
    std::array<float, 2> chromeLight{};
    float chromeTimeTerm = 0.0f;
    mu::SkinningTextureCoordinates textureCoordinates = mu::SkinningTextureCoordinates::Mesh;
    bool translate = false;
    bool lightEnabled = false;
};

struct RenderTapeSkinnedDraw
{
    std::uint32_t textureId = 0;
    RenderTapeState state{};
    std::vector<mu::SkinnedVertex3D> vertices;
    RenderTapeSkinningSnapshot skinning;
};

struct RenderTapeTextDraw
{
    std::vector<mu::Vertex2D> vertices;
    void* atlasTexture = nullptr;
    void* sampler = nullptr;
};

struct RenderTapeClear
{
    bool color = false;
    bool depth = false;
    bool stencil = false;
    RenderTapeState state{};
};

enum class RenderTapeCommandType : std::uint8_t
{
    Draw,
    SkinnedDraw,
    TextDraw,
    Clear,
};

struct RenderTapeCommand
{
    RenderTapeCommandType type = RenderTapeCommandType::Draw;
    RenderTapeDraw draw{};
    RenderTapeSkinnedDraw skinnedDraw{};
    RenderTapeTextDraw textDraw{};
    RenderTapeClear clear{};
};

struct RenderTapeBlock
{
    RenderTapePass pass = RenderTapePass::World;
    SessionFogPassConstants fog{};
    std::vector<RenderTapeCommand> commands;
};

class SessionRenderTape
{
public:
    SessionRenderTape() = default;
    explicit SessionRenderTape(std::vector<RenderTapeBlock> blocks) : m_blocks(std::move(blocks)) {}

    [[nodiscard]] bool Empty() const noexcept { return m_blocks.empty(); }
    [[nodiscard]] std::size_t DrawCount() const noexcept;
    [[nodiscard]] const std::vector<RenderTapeBlock>& Blocks() const noexcept { return m_blocks; }

    [[nodiscard]] bool Replay() const noexcept;

private:
    std::vector<RenderTapeBlock> m_blocks;
};

class SessionRenderTapeRecording
{
public:
    [[nodiscard]] bool BeginPass(RenderTapePass pass, const SessionFogPassConstants& fog) noexcept;
    [[nodiscard]] bool EndPass() noexcept;
    [[nodiscard]] bool AppendDraw(RenderTapeDraw draw) noexcept;
    [[nodiscard]] bool AppendSkinnedDraw(RenderTapeSkinnedDraw draw) noexcept;
    [[nodiscard]] bool AppendTextDraw(RenderTapeTextDraw draw) noexcept;
    [[nodiscard]] bool AppendClear(bool color, bool depth, bool stencil, const RenderTapeState& state) noexcept;
    [[nodiscard]] std::optional<SessionRenderTape> Finalize() noexcept;
    void Reset() noexcept;

    [[nodiscard]] bool IsRecording() const noexcept { return m_currentBlock.has_value(); }

private:
    std::vector<RenderTapeBlock> m_blocks;
    std::optional<std::size_t> m_currentBlock;
};

} // namespace mu::pipeline
