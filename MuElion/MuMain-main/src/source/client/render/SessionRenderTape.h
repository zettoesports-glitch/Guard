#pragma once

#include "client/render/RenderTapeTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
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

    RenderCompareFunction depthFunc = RenderCompareFunction::LessEqual;
    RenderCompareFunction alphaFunc = RenderCompareFunction::Greater;
    float alphaRef = 0.0f;
    RenderBlendFactor blendSource = RenderBlendFactor::SourceAlpha;
    RenderBlendFactor blendDestination = RenderBlendFactor::OneMinusSourceAlpha;
    RenderFrontFace frontFace = RenderFrontFace::CounterClockwise;

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

    std::array<float, 16> modelView{};
    std::array<float, 16> projection{};
};

struct RenderTapeDraw
{
    LegacyPrimitive primitive = LegacyPrimitive::Triangles;
    std::uint32_t textureId = 0;
    RenderTapeState state{};
    std::vector<RenderTapeVertex> vertices;
};

struct RenderTapeBlock
{
    RenderTapePass pass = RenderTapePass::World;
    SessionFogPassConstants fog{};
    std::vector<RenderTapeDraw> draws;
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
    [[nodiscard]] bool AppendDraw(RenderTapeDraw draw) noexcept;
    [[nodiscard]] std::optional<SessionRenderTape> Finalize() noexcept;
    void Reset() noexcept;

    [[nodiscard]] bool IsRecording() const noexcept { return m_currentBlock.has_value(); }

private:
    std::vector<RenderTapeBlock> m_blocks;
    std::optional<std::size_t> m_currentBlock;
};

} // namespace mu::pipeline
