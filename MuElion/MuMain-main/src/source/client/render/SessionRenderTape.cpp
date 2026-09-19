#include "stdafx.h"
#include "client/render/SessionRenderTape.h"

#include "Render/Renderer/MuRenderer.h"

#include <algorithm>

namespace mu::pipeline
{

namespace
{
bool ApplyBlend(RenderBlendFactor source, RenderBlendFactor destination) noexcept
{
    auto& renderer = mu::GetRenderer();
    if (source == RenderBlendFactor::SourceAlpha && destination == RenderBlendFactor::OneMinusSourceAlpha)
        renderer.SetBlendMode(mu::BlendMode::Alpha);
    else if (source == RenderBlendFactor::SourceAlpha && destination == RenderBlendFactor::One)
        renderer.SetBlendMode(mu::BlendMode::Additive);
    else if (source == RenderBlendFactor::Zero && destination == RenderBlendFactor::OneMinusSourceColor)
        renderer.SetBlendMode(mu::BlendMode::Subtract);
    else if (source == RenderBlendFactor::OneMinusDestinationColor && destination == RenderBlendFactor::Zero)
        renderer.SetBlendMode(mu::BlendMode::InverseColor);
    else if (source == RenderBlendFactor::One && destination == RenderBlendFactor::OneMinusSourceAlpha)
        renderer.SetBlendMode(mu::BlendMode::Mixed);
    else if (source == RenderBlendFactor::Zero && destination == RenderBlendFactor::SourceColor)
        renderer.SetBlendMode(mu::BlendMode::LightMap);
    else if (source == RenderBlendFactor::One && destination == RenderBlendFactor::One)
        renderer.SetBlendMode(mu::BlendMode::Glow);
    else
        return false;
    return true;
}

std::uint32_t PackColor(const std::array<float, 4>& color) noexcept
{
    const auto byte = [](float value) {
        return static_cast<std::uint32_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    return (byte(color[3]) << 24u) | (byte(color[2]) << 16u) | (byte(color[1]) << 8u) | byte(color[0]);
}

void ApplyState(const RenderTapeState& state) noexcept
{
    auto& renderer = mu::GetRenderer();

    renderer.SetDepthTest(state.depthTestEnabled);
    renderer.SetDepthMask(state.depthWriteEnabled);
    renderer.SetCullFace(state.cullEnabled);
    renderer.SetAlphaTest(state.alphaTestEnabled);
    renderer.SetFogEnabled(state.fogEnabled);
    renderer.SetStencilTest(state.stencilEnabled);
    renderer.SetScissorEnabled(state.scissorEnabled);
    renderer.SetDepthFunc(static_cast<int>(state.depthFunc));
    renderer.SetAlphaFunc(static_cast<int>(state.alphaFunc), state.alphaRef);
    renderer.SetFrontFace(static_cast<int>(state.frontFace));
    renderer.SetColorMask(state.colorMaskR, state.colorMaskG, state.colorMaskB, state.colorMaskA);

    if (state.blendEnabled)
    {
        (void)ApplyBlend(state.blendSource, state.blendDestination);
    }
    else
    {
        renderer.DisableBlend();
    }

    mu::FogParams fog{};
    fog.mode = state.fogMode;
    fog.start = state.fogStart;
    fog.end = state.fogEnd;
    fog.density = state.fogDensity;
    std::copy(state.fogColor.begin(), state.fogColor.end(), fog.color);
    renderer.SetFog(fog);

    if (state.viewport.width > 0 && state.viewport.height > 0)
        renderer.SetViewport(state.viewport.x, state.viewport.y, state.viewport.width, state.viewport.height);
    if (state.scissor.width > 0 && state.scissor.height > 0)
        renderer.SetScissor(state.scissor.x, state.scissor.y, state.scissor.width, state.scissor.height);

    renderer.SetMatrixMode(static_cast<int>(LegacyMatrixMode::Projection));
    renderer.LoadMatrix(state.projection.data());
    renderer.SetMatrixMode(static_cast<int>(LegacyMatrixMode::ModelView));
    renderer.LoadMatrix(state.modelView.data());

}

bool ReplayDraw(const RenderTapeDraw& draw) noexcept
{
    auto& renderer = mu::GetRenderer();
    const auto& state = draw.state;
    ApplyState(state);

    const std::uint32_t textureId = state.textureEnabled ? draw.textureId : 0u;
    renderer.BindTexture(static_cast<int>(textureId));

    if (draw.screenSpace2D)
    {
        std::vector<mu::Vertex2D> vertices2D;
        vertices2D.reserve(draw.vertices.size());
        for (const auto& vertex : draw.vertices)
        {
            vertices2D.push_back({vertex.position[0], vertex.position[1],
                                  vertex.texCoord[0], vertex.texCoord[1],
                                  PackColor(vertex.color)});
        }
        if (vertices2D.size() % 4 != 0)
            return false;
        renderer.RenderQuad2D(vertices2D, textureId);
        return true;
    }

    std::vector<mu::Vertex3D> vertices;
    vertices.reserve(draw.vertices.size());
    for (const auto& vertex : draw.vertices)
    {
        vertices.push_back({vertex.position[0], vertex.position[1], vertex.position[2],
                            vertex.normal[0], vertex.normal[1], vertex.normal[2],
                            vertex.texCoord[0], vertex.texCoord[1], PackColor(vertex.color)});
    }

    switch (draw.primitive)
    {
    case LegacyPrimitive::Triangles:
        if (vertices.size() % 3 != 0) return false;
        renderer.RenderTriangles(vertices, textureId);
        return true;
    case LegacyPrimitive::Quads:
        if (vertices.size() % 4 != 0) return false;
        renderer.RenderQuad3D(vertices, textureId);
        return true;
    case LegacyPrimitive::QuadStrip:
        renderer.RenderQuadStrip(vertices, textureId);
        return true;
    case LegacyPrimitive::Lines:
        renderer.RenderLines(vertices, textureId);
        return true;
    default:
        return false;
    }
}

bool ReplaySkinnedDraw(const RenderTapeSkinnedDraw& draw) noexcept
{
    auto& renderer = mu::GetRenderer();
    ApplyState(draw.state);

    const std::uint32_t textureId = draw.state.textureEnabled ? draw.textureId : 0u;
    renderer.BindTexture(static_cast<int>(textureId));

    mu::SkinningParameters parameters{
        .boneMatrices = draw.skinning.boneMatrices,
        .paletteVersion = draw.skinning.paletteVersion,
        .bodyOrigin = {draw.skinning.bodyOrigin[0], draw.skinning.bodyOrigin[1], draw.skinning.bodyOrigin[2]},
        .bodyScale = draw.skinning.bodyScale,
        .boneScale = draw.skinning.boneScale,
        .restPoseScale = draw.skinning.restPoseScale,
        .lightDirection = {draw.skinning.lightDirection[0], draw.skinning.lightDirection[1], draw.skinning.lightDirection[2]},
        .textureCoordinateOffset = {draw.skinning.textureCoordinateOffset[0], draw.skinning.textureCoordinateOffset[1]},
        .chromeWave = draw.skinning.chromeWave,
        .chromeWave2 = draw.skinning.chromeWave2,
        .chromeLight = {draw.skinning.chromeLight[0], draw.skinning.chromeLight[1]},
        .chromeTimeTerm = draw.skinning.chromeTimeTerm,
        .textureCoordinates = draw.skinning.textureCoordinates,
        .translate = draw.skinning.translate,
        .lightEnabled = draw.skinning.lightEnabled,
    };

    return renderer.RenderSkinnedTriangles(draw.vertices, textureId, parameters);
}
} // namespace

std::size_t SessionRenderTape::DrawCount() const noexcept
{
    std::size_t count = 0;
    for (const auto& block : m_blocks)
        for (const auto& command : block.commands)
            if (command.type == RenderTapeCommandType::Draw ||
                command.type == RenderTapeCommandType::SkinnedDraw)
                ++count;
    return count;
}

bool SessionRenderTape::Replay() const noexcept
{
    for (const auto& block : m_blocks)
    {
        for (const auto& command : block.commands)
        {
            if (command.type == RenderTapeCommandType::Draw)
            {
                if (!ReplayDraw(command.draw))
                    return false;
                continue;
            }

            if (command.type == RenderTapeCommandType::SkinnedDraw)
            {
                if (!ReplaySkinnedDraw(command.skinnedDraw))
                    return false;
                continue;
            }

            const auto& clear = command.clear;
            const auto& state = clear.state;
            auto& renderer = mu::GetRenderer();
            renderer.SetClearColor(state.clearColor[0], state.clearColor[1],
                                   state.clearColor[2], state.clearColor[3]);
            if (clear.color)
                renderer.ClearScreen();
            else if (clear.depth)
                renderer.ClearDepthBuffer();
            // The current SDL GPU abstraction has no standalone stencil-clear
            // entry point yet; preserve the command and state for the backend
            // extension instead of silently reordering it.
        }
    }
    return true;
}

bool SessionRenderTapeRecording::BeginPass(RenderTapePass pass, const SessionFogPassConstants& fog) noexcept
{
    RenderTapeBlock block{};
    block.pass = pass;
    block.fog = fog;
    m_blocks.push_back(std::move(block));
    m_currentBlock = m_blocks.size() - 1;
    return true;
}

bool SessionRenderTapeRecording::EndPass() noexcept
{
    if (!m_currentBlock)
        return false;
    m_currentBlock.reset();
    return true;
}

bool SessionRenderTapeRecording::AppendDraw(RenderTapeDraw draw) noexcept
{
    if (!m_currentBlock || *m_currentBlock >= m_blocks.size())
        return false;

    RenderTapeCommand command{};
    command.type = RenderTapeCommandType::Draw;
    command.draw = std::move(draw);
    m_blocks[*m_currentBlock].commands.push_back(std::move(command));
    return true;
}

bool SessionRenderTapeRecording::AppendSkinnedDraw(RenderTapeSkinnedDraw draw) noexcept
{
    if (!m_currentBlock || *m_currentBlock >= m_blocks.size())
        return false;

    RenderTapeCommand command{};
    command.type = RenderTapeCommandType::SkinnedDraw;
    command.skinnedDraw = std::move(draw);
    m_blocks[*m_currentBlock].commands.push_back(std::move(command));
    return true;
}

bool SessionRenderTapeRecording::AppendClear(bool color, bool depth, bool stencil,
                                             const RenderTapeState& state) noexcept
{
    if (!m_currentBlock || *m_currentBlock >= m_blocks.size())
        return false;

    RenderTapeCommand command{};
    command.type = RenderTapeCommandType::Clear;
    command.clear.color = color;
    command.clear.depth = depth;
    command.clear.stencil = stencil;
    command.clear.state = state;
    m_blocks[*m_currentBlock].commands.push_back(std::move(command));
    return true;
}

std::optional<SessionRenderTape> SessionRenderTapeRecording::Finalize() noexcept
{
    if (m_blocks.empty())
        return std::nullopt;

    SessionRenderTape result(std::move(m_blocks));
    m_blocks.clear();
    m_currentBlock.reset();
    return result;
}

void SessionRenderTapeRecording::Reset() noexcept
{
    m_blocks.clear();
    m_currentBlock.reset();
}

} // namespace mu::pipeline
