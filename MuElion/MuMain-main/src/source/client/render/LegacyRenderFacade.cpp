#include "stdafx.h"
#include "client/render/LegacyRenderFacade.h"

#include "Render/Renderer/MuRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mu::pipeline
{

namespace
{
template <typename T>
bool ReadScalar(const std::vector<std::byte>& bytes, std::size_t offset, T& value) noexcept
{
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset)
        return false;
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return true;
}

std::size_t ScalarSize(RenderClientArrayScalarType type) noexcept
{
    switch (type)
    {
    case RenderClientArrayScalarType::Byte:
    case RenderClientArrayScalarType::UnsignedByte:
        return 1;
    case RenderClientArrayScalarType::Short:
    case RenderClientArrayScalarType::UnsignedShort:
        return 2;
    case RenderClientArrayScalarType::Int:
    case RenderClientArrayScalarType::UnsignedInt:
    case RenderClientArrayScalarType::Float:
        return 4;
    case RenderClientArrayScalarType::Double:
        return 8;
    default:
        return 0;
    }
}

template <typename ClientArrayT>
bool ReadComponent(const ClientArrayT& array, int index, std::uint32_t component, float& out) noexcept
{
    const std::size_t scalarSize = ScalarSize(array.scalarType);
    if (scalarSize == 0 || component >= array.componentCount || index < 0)
        return false;

    const std::size_t packedStride = scalarSize * array.componentCount;
    const std::size_t stride = array.stride > 0 ? static_cast<std::size_t>(array.stride) : packedStride;
    const std::size_t offset = static_cast<std::size_t>(index) * stride + static_cast<std::size_t>(component) * scalarSize;

    switch (array.scalarType)
    {
    case RenderClientArrayScalarType::Float: {
        float value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = value;
        return true;
    }
    case RenderClientArrayScalarType::Double: {
        double value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = static_cast<float>(value);
        return true;
    }
    case RenderClientArrayScalarType::Byte: {
        std::int8_t value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = array.normalized ? std::max(-1.0f, static_cast<float>(value) / 127.0f) : static_cast<float>(value);
        return true;
    }
    case RenderClientArrayScalarType::UnsignedByte: {
        std::uint8_t value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = array.normalized ? static_cast<float>(value) / 255.0f : static_cast<float>(value);
        return true;
    }
    case RenderClientArrayScalarType::Short: {
        std::int16_t value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = array.normalized ? std::max(-1.0f, static_cast<float>(value) / 32767.0f) : static_cast<float>(value);
        return true;
    }
    case RenderClientArrayScalarType::UnsignedShort: {
        std::uint16_t value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = array.normalized ? static_cast<float>(value) / 65535.0f : static_cast<float>(value);
        return true;
    }
    case RenderClientArrayScalarType::Int: {
        std::int32_t value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = static_cast<float>(value);
        return true;
    }
    case RenderClientArrayScalarType::UnsignedInt: {
        std::uint32_t value{};
        if (!ReadScalar(array.bytes, offset, value)) return false;
        out = static_cast<float>(value);
        return true;
    }
    default:
        return false;
    }
}
} // namespace

LegacyRenderFacade::LegacyRenderFacade(LogicalRenderAssetTable& assets) noexcept
    : m_assets(assets)
{
    m_state.modelView.fill(0.0f);
    m_state.projection.fill(0.0f);
    m_state.modelView[0] = m_state.modelView[5] = m_state.modelView[10] = m_state.modelView[15] = 1.0f;
    m_state.projection[0] = m_state.projection[5] = m_state.projection[10] = m_state.projection[15] = 1.0f;
}

bool LegacyRenderFacade::BeginPass(RenderTapePass pass, const SessionFogPassConstants& fog) noexcept
{
    if (m_insidePrimitive || m_recording.IsRecording())
        return false;

    m_state.fogEnabled = fog.enabled;
    m_state.fogMode = static_cast<int>(fog.mode);
    m_state.fogStart = fog.start;
    m_state.fogEnd = fog.end;
    m_state.fogDensity = fog.density;
    m_state.fogColor = fog.color;
    return m_recording.BeginPass(pass, fog);
}

bool LegacyRenderFacade::EndPass() noexcept
{
    if (m_insidePrimitive && !End())
        return false;
    return m_recording.EndPass();
}

std::optional<SessionRenderTape> LegacyRenderFacade::Finalize() noexcept
{
    if (m_insidePrimitive && !End())
    {
        m_boneMatrices.clear();
        return std::nullopt;
    }
    if (m_recording.IsRecording() && !m_recording.EndPass())
    {
        m_boneMatrices.clear();
        return std::nullopt;
    }

    auto tape = m_recording.Finalize();
    m_boneMatrices.clear();
    return tape;
}

bool LegacyRenderFacade::Begin(LegacyPrimitive primitive) noexcept
{
    if (m_insidePrimitive)
        return false;

    m_primitive = primitive;
    m_vertices.clear();
    m_insidePrimitive = true;
    return true;
}

bool LegacyRenderFacade::End() noexcept
{
    if (!m_insidePrimitive)
        return false;

    m_insidePrimitive = false;
    return EmitPrimitiveDraw(m_primitive, m_vertices);
}

bool LegacyRenderFacade::Vertex3(float x, float y, float z) noexcept
{
    if (!m_insidePrimitive)
        return false;

    m_current.position = {x, y, z};
    m_vertices.push_back(m_current);
    return true;
}

bool LegacyRenderFacade::Color4(float r, float g, float b, float a) noexcept
{
    m_current.color = {r, g, b, a};
    return true;
}

bool LegacyRenderFacade::Normal3(float x, float y, float z) noexcept
{
    m_current.normal = {x, y, z};
    return true;
}

bool LegacyRenderFacade::TexCoord2(float u, float v) noexcept
{
    m_current.texCoord = {u, v};
    return true;
}

std::uint32_t LegacyRenderFacade::PackColor(const std::array<float, 4>& color) noexcept
{
    const auto byte = [](float v) {
        return static_cast<std::uint32_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    const std::uint32_t r = byte(color[0]);
    const std::uint32_t g = byte(color[1]);
    const std::uint32_t b = byte(color[2]);
    const std::uint32_t a = byte(color[3]);
    return (a << 24u) | (b << 16u) | (g << 8u) | r;
}

bool LegacyRenderFacade::EmitExpandedTriangles(
    LegacyPrimitive primitive, std::span<const RenderTapeVertex> vertices) noexcept
{
    if (vertices.empty())
        return true;

    std::vector<RenderTapeVertex> expanded;

    if (primitive == LegacyPrimitive::TriangleFan)
    {
        if (vertices.size() < 3)
            return true;

        expanded.reserve((vertices.size() - 2) * 3);
        for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
        {
            expanded.push_back(vertices[0]);
            expanded.push_back(vertices[i]);
            expanded.push_back(vertices[i + 1]);
        }
        return EmitPrimitiveDraw(LegacyPrimitive::Triangles, expanded);
    }

    if (primitive == LegacyPrimitive::TriangleStrip)
    {
        if (vertices.size() < 3)
            return true;

        expanded.reserve((vertices.size() - 2) * 3);
        for (std::size_t i = 0; i + 2 < vertices.size(); ++i)
        {
            if ((i & 1u) == 0u)
            {
                expanded.push_back(vertices[i]);
                expanded.push_back(vertices[i + 1]);
            }
            else
            {
                expanded.push_back(vertices[i + 1]);
                expanded.push_back(vertices[i]);
            }
            expanded.push_back(vertices[i + 2]);
        }
        return EmitPrimitiveDraw(LegacyPrimitive::Triangles, expanded);
    }

    if (primitive == LegacyPrimitive::LineStrip || primitive == LegacyPrimitive::LineLoop)
    {
        if (vertices.size() < 2)
            return true;

        const std::size_t pairCount =
            primitive == LegacyPrimitive::LineLoop ? vertices.size() : vertices.size() - 1;
        expanded.reserve(pairCount * 2);
        for (std::size_t i = 0; i < pairCount; ++i)
        {
            expanded.push_back(vertices[i]);
            expanded.push_back(vertices[(i + 1) % vertices.size()]);
        }
        return EmitPrimitiveDraw(LegacyPrimitive::Lines, expanded);
    }

    return false;
}

bool LegacyRenderFacade::EmitPrimitiveDraw(LegacyPrimitive primitive, std::span<const RenderTapeVertex> vertices) noexcept
{
    if (vertices.empty())
        return true;

    if (primitive == LegacyPrimitive::TriangleFan ||
        primitive == LegacyPrimitive::TriangleStrip ||
        primitive == LegacyPrimitive::LineStrip ||
        primitive == LegacyPrimitive::LineLoop)
    {
        return EmitExpandedTriangles(primitive, vertices);
    }

    if (m_recording.IsRecording())
    {
        mu::GetRenderer().GetMatrix(static_cast<int>(LegacyMatrixMode::ModelView), m_state.modelView.data());
        mu::GetRenderer().GetMatrix(static_cast<int>(LegacyMatrixMode::Projection), m_state.projection.data());

        RenderTapeDraw draw{};
        draw.primitive = primitive;
        draw.textureId = m_boundTextureId;
        draw.state = m_state;
        draw.vertices.assign(vertices.begin(), vertices.end());
        return m_recording.AppendDraw(std::move(draw));
    }

    std::vector<mu::Vertex3D> converted;
    converted.reserve(vertices.size());
    for (const auto& vertex : vertices)
    {
        converted.push_back({vertex.position[0], vertex.position[1], vertex.position[2],
                             vertex.normal[0], vertex.normal[1], vertex.normal[2],
                             vertex.texCoord[0], vertex.texCoord[1], PackColor(vertex.color)});
    }

    switch (primitive)
    {
    case LegacyPrimitive::Triangles:
        if (converted.size() % 3 != 0) return false;
        mu::GetRenderer().RenderTriangles(converted, m_boundTextureId);
        return true;
    case LegacyPrimitive::Quads:
        if (converted.size() % 4 != 0) return false;
        mu::GetRenderer().RenderQuad3D(converted, m_boundTextureId);
        return true;
    case LegacyPrimitive::QuadStrip:
        mu::GetRenderer().RenderQuadStrip(converted, m_boundTextureId);
        return true;
    case LegacyPrimitive::Lines:
        mu::GetRenderer().RenderLines(converted, m_boundTextureId);
        return true;
    default:
        return false;
    }
}

bool LegacyRenderFacade::SubmitTriangles(std::span<const mu::Vertex3D> vertices, std::uint32_t textureId) noexcept
{
    if (vertices.empty())
        return true;

    if (!m_recording.IsRecording())
    {
        mu::GetRenderer().RenderTriangles(vertices, textureId);
        return true;
    }

    std::vector<RenderTapeVertex> tapeVertices;
    tapeVertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        RenderTapeVertex out{};
        out.position = {v.x, v.y, v.z};
        out.normal = {v.nx, v.ny, v.nz};
        out.texCoord = {v.u, v.v};
        out.color = {
            static_cast<float>(v.color & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 8u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 16u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 24u) & 0xffu) / 255.0f,
        };
        tapeVertices.push_back(out);
    }

    const std::uint32_t previousTexture = m_boundTextureId;
    m_boundTextureId = textureId;
    const bool result = EmitPrimitiveDraw(LegacyPrimitive::Triangles, tapeVertices);
    m_boundTextureId = previousTexture;
    return result;
}

bool LegacyRenderFacade::SubmitQuad3D(std::span<const mu::Vertex3D> vertices, std::uint32_t textureId) noexcept
{
    if (vertices.empty())
        return true;

    if (!m_recording.IsRecording())
    {
        mu::GetRenderer().RenderQuad3D(vertices, textureId);
        return true;
    }

    std::vector<RenderTapeVertex> tapeVertices;
    tapeVertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        RenderTapeVertex out{};
        out.position = {v.x, v.y, v.z};
        out.normal = {v.nx, v.ny, v.nz};
        out.texCoord = {v.u, v.v};
        out.color = {
            static_cast<float>(v.color & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 8u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 16u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 24u) & 0xffu) / 255.0f,
        };
        tapeVertices.push_back(out);
    }

    const std::uint32_t previousTexture = m_boundTextureId;
    m_boundTextureId = textureId;
    const bool result = EmitPrimitiveDraw(LegacyPrimitive::Quads, tapeVertices);
    m_boundTextureId = previousTexture;
    return result;
}

bool LegacyRenderFacade::SubmitQuad2D(std::span<const mu::Vertex2D> vertices, std::uint32_t textureId) noexcept
{
    if (vertices.empty())
        return true;

    if (!m_recording.IsRecording())
    {
        mu::GetRenderer().RenderQuad2D(vertices, textureId);
        return true;
    }

    mu::GetRenderer().GetMatrix(static_cast<int>(LegacyMatrixMode::ModelView), m_state.modelView.data());
    mu::GetRenderer().GetMatrix(static_cast<int>(LegacyMatrixMode::Projection), m_state.projection.data());

    RenderTapeDraw draw{};
    draw.primitive = LegacyPrimitive::Quads;
    draw.screenSpace2D = true;
    draw.textureId = textureId;
    draw.state = m_state;
    draw.vertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        RenderTapeVertex out{};
        out.position = {v.x, v.y, 0.0f};
        out.texCoord = {v.u, v.v};
        out.color = {
            static_cast<float>(v.color & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 8u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 16u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 24u) & 0xffu) / 255.0f,
        };
        draw.vertices.push_back(out);
    }
    return m_recording.AppendDraw(std::move(draw));
}

bool LegacyRenderFacade::SubmitTextTriangles(std::span<const mu::Vertex2D> vertices,
                                                   void* atlasTexture, void* sampler) noexcept
{
    if (vertices.empty())
        return true;
    if (atlasTexture == nullptr)
        return false;

    if (!m_recording.IsRecording())
    {
        mu::GetRenderer().SubmitTextTriangles(vertices, atlasTexture, sampler);
        return true;
    }

    RenderTapeTextDraw draw{};
    draw.vertices.assign(vertices.begin(), vertices.end());
    draw.atlasTexture = atlasTexture;
    draw.sampler = sampler;
    return m_recording.AppendTextDraw(std::move(draw));
}

bool LegacyRenderFacade::SubmitLines(std::span<const mu::Vertex3D> vertices, std::uint32_t textureId) noexcept
{
    if (vertices.empty())
        return true;

    if (!m_recording.IsRecording())
    {
        mu::GetRenderer().RenderLines(vertices, textureId);
        return true;
    }

    std::vector<RenderTapeVertex> tapeVertices;
    tapeVertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        RenderTapeVertex out{};
        out.position = {v.x, v.y, v.z};
        out.normal = {v.nx, v.ny, v.nz};
        out.texCoord = {v.u, v.v};
        out.color = {
            static_cast<float>(v.color & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 8u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 16u) & 0xffu) / 255.0f,
            static_cast<float>((v.color >> 24u) & 0xffu) / 255.0f,
        };
        tapeVertices.push_back(out);
    }

    const std::uint32_t previousTexture = m_boundTextureId;
    m_boundTextureId = textureId;
    const bool result = EmitPrimitiveDraw(LegacyPrimitive::Lines, tapeVertices);
    m_boundTextureId = previousTexture;
    return result;
}

bool LegacyRenderFacade::SubmitSkinnedTriangles(std::span<const mu::SkinnedVertex3D> vertices,
                                                std::uint32_t textureId,
                                                const mu::SkinningParameters& parameters) noexcept
{
    if (vertices.empty())
        return true;

    if (!m_recording.IsRecording())
        return mu::GetRenderer().RenderSkinnedTriangles(vertices, textureId, parameters);

    RenderTapeSkinnedDraw draw{};
    draw.textureId = textureId;
    draw.state = m_state;
    draw.vertices.assign(vertices.begin(), vertices.end());
    draw.skinning.boneMatrices.assign(parameters.boneMatrices.begin(), parameters.boneMatrices.end());
    draw.skinning.paletteVersion = parameters.paletteVersion;
    draw.skinning.bodyOrigin = {parameters.bodyOrigin[0], parameters.bodyOrigin[1], parameters.bodyOrigin[2]};
    draw.skinning.bodyScale = parameters.bodyScale;
    draw.skinning.boneScale = parameters.boneScale;
    draw.skinning.restPoseScale = parameters.restPoseScale;
    draw.skinning.lightDirection = {parameters.lightDirection[0], parameters.lightDirection[1], parameters.lightDirection[2]};
    draw.skinning.textureCoordinateOffset = {parameters.textureCoordinateOffset[0], parameters.textureCoordinateOffset[1]};
    draw.skinning.chromeWave = parameters.chromeWave;
    draw.skinning.chromeWave2 = parameters.chromeWave2;
    draw.skinning.chromeLight = {parameters.chromeLight[0], parameters.chromeLight[1]};
    draw.skinning.chromeTimeTerm = parameters.chromeTimeTerm;
    draw.skinning.textureCoordinates = parameters.textureCoordinates;
    draw.skinning.translate = parameters.translate;
    draw.skinning.lightEnabled = parameters.lightEnabled;
    return m_recording.AppendSkinnedDraw(std::move(draw));
}

bool LegacyRenderFacade::SetClientArray(RenderClientArraySemantic semantic, std::span<const std::byte> bytes,
                                        std::uint32_t componentCount, RenderClientArrayScalarType scalarType,
                                        int stride, bool normalized) noexcept
{
    ClientArray* target = nullptr;
    switch (semantic)
    {
    case RenderClientArraySemantic::Vertex: target = &m_vertexArray; break;
    case RenderClientArraySemantic::Normal: target = &m_normalArray; break;
    case RenderClientArraySemantic::Color: target = &m_colorArray; break;
    case RenderClientArraySemantic::TextureCoordinate: target = &m_texCoordArray; break;
    }

    if (!target || componentCount == 0 || componentCount > 4 || ScalarSize(scalarType) == 0)
        return false;

    target->bytes.assign(bytes.begin(), bytes.end());
    target->componentCount = componentCount;
    target->scalarType = scalarType;
    target->stride = stride;
    target->normalized = normalized;
    return true;
}

void LegacyRenderFacade::EnableClientArray(RenderClientArraySemantic semantic) noexcept
{
    switch (semantic)
    {
    case RenderClientArraySemantic::Vertex: m_vertexArray.enabled = true; break;
    case RenderClientArraySemantic::Normal: m_normalArray.enabled = true; break;
    case RenderClientArraySemantic::Color: m_colorArray.enabled = true; break;
    case RenderClientArraySemantic::TextureCoordinate: m_texCoordArray.enabled = true; break;
    }
}

void LegacyRenderFacade::DisableClientArray(RenderClientArraySemantic semantic) noexcept
{
    switch (semantic)
    {
    case RenderClientArraySemantic::Vertex: m_vertexArray.enabled = false; break;
    case RenderClientArraySemantic::Normal: m_normalArray.enabled = false; break;
    case RenderClientArraySemantic::Color: m_colorArray.enabled = false; break;
    case RenderClientArraySemantic::TextureCoordinate: m_texCoordArray.enabled = false; break;
    }
}

bool LegacyRenderFacade::ReadArrayVertex(int index, RenderTapeVertex& vertex) const noexcept
{
    if (!m_vertexArray.enabled || m_vertexArray.componentCount < 2)
        return false;

    for (std::uint32_t c = 0; c < std::min<std::uint32_t>(3, m_vertexArray.componentCount); ++c)
        if (!ReadComponent(m_vertexArray, index, c, vertex.position[c])) return false;

    if (m_normalArray.enabled)
        for (std::uint32_t c = 0; c < std::min<std::uint32_t>(3, m_normalArray.componentCount); ++c)
            if (!ReadComponent(m_normalArray, index, c, vertex.normal[c])) return false;

    if (m_texCoordArray.enabled)
        for (std::uint32_t c = 0; c < std::min<std::uint32_t>(2, m_texCoordArray.componentCount); ++c)
            if (!ReadComponent(m_texCoordArray, index, c, vertex.texCoord[c])) return false;

    if (m_colorArray.enabled)
        for (std::uint32_t c = 0; c < std::min<std::uint32_t>(4, m_colorArray.componentCount); ++c)
            if (!ReadComponent(m_colorArray, index, c, vertex.color[c])) return false;

    return true;
}

bool LegacyRenderFacade::ReadArrayVertices(int first, int count,
                                           std::span<RenderTapeVertex> out) noexcept
{
    if (first < 0 || count < 0 || out.size() < static_cast<std::size_t>(count))
        return false;

    for (int i = 0; i < count; ++i)
    {
        if (!ReadArrayVertex(first + i, out[static_cast<std::size_t>(i)]))
            return false;
    }
    return true;
}

bool LegacyRenderFacade::CountClassifiedArrayTriangles(
    LegacyPrimitive primitive, int first, int count,
    std::uint64_t& frontFacing, std::uint64_t& backFacing) noexcept
{
    frontFacing = 0;
    backFacing = 0;

    if (first < 0 || count < 0)
        return false;

    std::vector<RenderTapeVertex> vertices(static_cast<std::size_t>(count));
    if (!ReadArrayVertices(first, count, vertices))
        return false;

    std::array<float, 16> modelView{};
    std::array<float, 16> projection{};
    mu::GetRenderer().GetMatrix(static_cast<int>(LegacyMatrixMode::ModelView), modelView.data());
    mu::GetRenderer().GetMatrix(static_cast<int>(LegacyMatrixMode::Projection), projection.data());

    const glm::mat4 mvp = glm::make_mat4(projection.data()) * glm::make_mat4(modelView.data());

    const auto classify = [&](const RenderTapeVertex& a,
                              const RenderTapeVertex& b,
                              const RenderTapeVertex& cVertex) {
        const glm::vec4 ca = mvp * glm::vec4(a.position[0], a.position[1], a.position[2], 1.0f);
        const glm::vec4 cb = mvp * glm::vec4(b.position[0], b.position[1], b.position[2], 1.0f);
        const glm::vec4 cc = mvp * glm::vec4(cVertex.position[0], cVertex.position[1], cVertex.position[2], 1.0f);

        if (std::abs(ca.w) < 1.0e-7f || std::abs(cb.w) < 1.0e-7f || std::abs(cc.w) < 1.0e-7f)
        {
            ++backFacing;
            return;
        }

        const glm::vec2 a2(ca.x / ca.w, ca.y / ca.w);
        const glm::vec2 b2(cb.x / cb.w, cb.y / cb.w);
        const glm::vec2 c2(cc.x / cc.w, cc.y / cc.w);
        const float signedArea =
            (b2.x - a2.x) * (c2.y - a2.y) -
            (b2.y - a2.y) * (c2.x - a2.x);

        const bool ccw = signedArea > 0.0f;
        const bool isFront =
            m_state.frontFace == RenderFrontFace::CounterClockwise ? ccw : !ccw;
        if (isFront)
            ++frontFacing;
        else
            ++backFacing;
    };

    switch (primitive)
    {
    case LegacyPrimitive::Triangles:
        for (std::size_t i = 0; i + 2 < vertices.size(); i += 3)
            classify(vertices[i], vertices[i + 1], vertices[i + 2]);
        return vertices.size() % 3 == 0;

    case LegacyPrimitive::TriangleFan:
        if (vertices.size() < 3)
            return true;
        for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
            classify(vertices[0], vertices[i], vertices[i + 1]);
        return true;

    case LegacyPrimitive::TriangleStrip:
        if (vertices.size() < 3)
            return true;
        for (std::size_t i = 0; i + 2 < vertices.size(); ++i)
        {
            if ((i & 1u) == 0u)
                classify(vertices[i], vertices[i + 1], vertices[i + 2]);
            else
                classify(vertices[i + 1], vertices[i], vertices[i + 2]);
        }
        return true;

    case LegacyPrimitive::Quads:
        if (vertices.size() % 4 != 0)
            return false;
        for (std::size_t i = 0; i + 3 < vertices.size(); i += 4)
        {
            classify(vertices[i], vertices[i + 1], vertices[i + 2]);
            classify(vertices[i], vertices[i + 2], vertices[i + 3]);
        }
        return true;

    default:
        return false;
    }
}

bool LegacyRenderFacade::DrawArrays(LegacyPrimitive primitive, int first, int count) noexcept
{
    if (first < 0 || count < 0)
        return false;

    std::vector<RenderTapeVertex> vertices(static_cast<std::size_t>(count));
    if (!ReadArrayVertices(first, count, vertices))
        return false;

    return EmitPrimitiveDraw(primitive, vertices);
}


bool LegacyRenderFacade::BuildTrustedGeometryDraw(
    const LogicalGeometryAssetLease& geometry, unsigned int first,
    unsigned int count, TrustedGeometryDraw& out) noexcept
{
    if (!geometry.IsValid())
        return false;

    const auto indices = geometry.Indices();
    const auto vertices = geometry.Vertices();

    if (!indices.empty())
    {
        const std::size_t begin = first;
        const std::size_t requested = count;
        if (begin > indices.size() || requested > indices.size() - begin)
            return false;

        for (std::size_t i = begin; i < begin + requested; ++i)
        {
            if (indices[i] >= vertices.size())
                return false;
        }
    }
    else
    {
        const std::size_t begin = first;
        const std::size_t requested = count;
        if (begin > vertices.size() || requested > vertices.size() - begin)
            return false;
    }

    out.geometry = geometry;
    out.first = first;
    out.count = count;
    return true;
}

bool LegacyRenderFacade::DrawGeometry(
    const LogicalGeometryAssetLease& geometry, unsigned int first,
    unsigned int count) noexcept
{
    TrustedGeometryDraw trusted{};
    if (!BuildTrustedGeometryDraw(geometry, first, count, trusted))
        return false;

    const auto indices = geometry.Indices();
    const auto vertices = geometry.Vertices();

    if (!indices.empty())
    {
        std::vector<mu::Vertex3D> expanded;
        expanded.reserve(count);
        for (std::size_t i = first; i < static_cast<std::size_t>(first) + count; ++i)
            expanded.push_back(vertices[indices[i]]);

        if (expanded.size() % 3u != 0u)
            return false;
        return SubmitTriangles(expanded, geometry.TextureId());
    }

    const auto subset = vertices.subspan(first, count);
    if (subset.size() % 3u != 0u)
        return false;
    return SubmitTriangles(subset, geometry.TextureId());
}

bool LegacyRenderFacade::AppendTrustedGeometryDrawBatch(
    std::span<const TrustedGeometryDraw> draws) noexcept
{
    for (const auto& draw : draws)
    {
        TrustedGeometryDraw verified{};
        if (!BuildTrustedGeometryDraw(
                draw.geometry, draw.first, draw.count, verified))
            return false;

        if (!DrawGeometry(verified.geometry, verified.first, verified.count))
            return false;
    }
    return true;
}

std::optional<unsigned int> LegacyRenderFacade::AppendBoneMatrices(
    std::span<const RenderTapeBoneMatrix> bones, bool reuseExisting) noexcept
{
    if (bones.empty())
        return 0u;

    if (bones.size() > static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()))
        return std::nullopt;

    if (reuseExisting && bones.size() <= m_boneMatrices.size())
    {
        for (std::size_t offset = 0; offset + bones.size() <= m_boneMatrices.size(); ++offset)
        {
            if (std::equal(bones.begin(), bones.end(), m_boneMatrices.begin() + offset,
                           [](const RenderTapeBoneMatrix& left, const RenderTapeBoneMatrix& right) {
                               return std::memcmp(&left, &right, sizeof(RenderTapeBoneMatrix)) == 0;
                           }))
            {
                return static_cast<unsigned int>(offset);
            }
        }
    }

    if (m_boneMatrices.size() >
        static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()) - bones.size())
        return std::nullopt;

    const auto offset = static_cast<unsigned int>(m_boneMatrices.size());
    m_boneMatrices.insert(m_boneMatrices.end(), bones.begin(), bones.end());
    return offset;
}

bool LegacyRenderFacade::DrawBmdGeometry(
    const LogicalGeometryAssetLease& geometry,
    unsigned int firstVertex, unsigned int vertexCount,
    unsigned int firstIndex, unsigned int indexCount,
    const RenderTapeBmdConstants& constants) noexcept
{
    if (!geometry.IsValid())
        return false;

    const auto sourceVertices = geometry.SkinnedVertices();
    if (sourceVertices.empty())
        return false;

    if (firstVertex > sourceVertices.size() ||
        vertexCount > sourceVertices.size() - firstVertex)
        return false;

    const auto indices = geometry.Indices();
    std::vector<mu::SkinnedVertex3D> vertices;

    if (!indices.empty())
    {
        if (firstIndex > indices.size() || indexCount > indices.size() - firstIndex)
            return false;

        vertices.reserve(indexCount);
        const std::size_t vertexEnd = static_cast<std::size_t>(firstVertex) + vertexCount;
        for (std::size_t i = firstIndex; i < static_cast<std::size_t>(firstIndex) + indexCount; ++i)
        {
            const std::size_t index = indices[i];
            if (index < firstVertex || index >= vertexEnd || index >= sourceVertices.size())
                return false;
            vertices.push_back(sourceVertices[index]);
        }
    }
    else
    {
        if (indexCount != 0)
            return false;
        vertices.assign(sourceVertices.begin() + firstVertex,
                        sourceVertices.begin() + firstVertex + vertexCount);
    }

    if (vertices.empty() || vertices.size() % 3u != 0u)
        return false;

    const std::size_t boneOffset = constants.bmdMode[1];
    if (boneOffset > m_boneMatrices.size())
        return false;

    const auto boneSpan =
        std::span<const RenderTapeBoneMatrix>(m_boneMatrices).subspan(boneOffset);
    const auto boneFloats = std::span<const float>(
        reinterpret_cast<const float*>(boneSpan.data()), boneSpan.size() * 12u);

    mu::SkinningParameters parameters{
        .boneMatrices = boneFloats,
        .paletteVersion = static_cast<std::uint32_t>(boneOffset),
        .bodyOrigin = {constants.bmdBodyOrigin[0],
                       constants.bmdBodyOrigin[1],
                       constants.bmdBodyOrigin[2]},
        .bodyScale = constants.bmdScale[0],
        .boneScale = constants.bmdScale[1],
        .restPoseScale = constants.bmdScale[2],
        .lightDirection = {constants.bmdLightPosition[0],
                           constants.bmdLightPosition[1],
                           constants.bmdLightPosition[2]},
        .textureCoordinateOffset = {constants.bmdUvAnimation[0],
                                    constants.bmdUvAnimation[1]},
        .chromeWave = constants.bmdUvAnimation[2],
        .chromeWave2 = constants.bmdUvAnimation[3],
        .chromeLight = {constants.bmdChromeLight[0],
                        constants.bmdChromeLight[1]},
        .chromeTimeTerm = constants.bmdChromeLight[3],
        .textureCoordinates =
            static_cast<mu::SkinningTextureCoordinates>(constants.bmdMode[2]),
        .translate = (constants.bmdMode[3] & RenderTapeBmdTranslate) != 0u,
        .lightEnabled = (constants.bmdMode[3] & RenderTapeBmdLighting) != 0u,
    };

    return SubmitSkinnedTriangles(vertices, geometry.TextureId(), parameters);
}

bool LegacyRenderFacade::DrawRigidInstances(
    const LogicalGeometryAssetLease& geometry,
    unsigned int firstVertex, unsigned int vertexCount,
    unsigned int firstIndex, unsigned int indexCount,
    std::span<const RenderTapeRigidInstance> instances,
    const RenderTapeBmdConstants& constants) noexcept
{
    if (!geometry.IsValid() || instances.empty())
        return false;

    const auto source = geometry.SkinnedVertices();
    if (source.empty() || firstVertex > source.size() ||
        vertexCount > source.size() - firstVertex)
        return false;

    const auto indices = geometry.Indices();
    std::vector<std::size_t> sourceIndices;
    if (!indices.empty())
    {
        if (firstIndex > indices.size() || indexCount > indices.size() - firstIndex)
            return false;
        sourceIndices.reserve(indexCount);
        const std::size_t vertexEnd = static_cast<std::size_t>(firstVertex) + vertexCount;
        for (std::size_t i = firstIndex; i < static_cast<std::size_t>(firstIndex) + indexCount; ++i)
        {
            const std::size_t index = indices[i];
            if (index < firstVertex || index >= vertexEnd || index >= source.size())
                return false;
            sourceIndices.push_back(index);
        }
    }
    else
    {
        if (indexCount != 0)
            return false;
        sourceIndices.reserve(vertexCount);
        for (std::size_t i = firstVertex; i < static_cast<std::size_t>(firstVertex) + vertexCount; ++i)
            sourceIndices.push_back(i);
    }

    if (sourceIndices.empty() || sourceIndices.size() % 3u != 0u)
        return false;

    std::vector<mu::Vertex3D> expanded;
    expanded.reserve(sourceIndices.size() * instances.size());

    const bool translate =
        (constants.bmdMode[3] & RenderTapeBmdTranslate) != 0u;
    const bool lighting =
        (constants.bmdMode[3] & RenderTapeBmdLighting) != 0u;
    const bool uvAnimation =
        (constants.bmdMode[3] & RenderTapeBmdUvAnimation) != 0u;
    const bool boneScalePath =
        (constants.bmdMode[3] & RenderTapeBmdBoneScalePath) != 0u;

    for (const auto& instance : instances)
    {
        for (const std::size_t sourceIndex : sourceIndices)
        {
            const auto& input = source[sourceIndex];

            const float inputScale =
                boneScalePath ? constants.bmdScale[1] : constants.bmdScale[0];
            const float px = input.x * inputScale;
            const float py = input.y * inputScale;
            const float pz = input.z * inputScale;

            float x = px * instance.transform0[0] +
                      py * instance.transform0[1] +
                      pz * instance.transform0[2] +
                      instance.transform0[3];
            float y = px * instance.transform1[0] +
                      py * instance.transform1[1] +
                      pz * instance.transform1[2] +
                      instance.transform1[3];
            float z = px * instance.transform2[0] +
                      py * instance.transform2[1] +
                      pz * instance.transform2[2] +
                      instance.transform2[3];

            if (translate)
            {
                x = x * constants.bmdScale[2] + constants.bmdBodyOrigin[0];
                y = y * constants.bmdScale[2] + constants.bmdBodyOrigin[1];
                z = z * constants.bmdScale[2] + constants.bmdBodyOrigin[2];
            }

            const float nx = input.nx * instance.transform0[0] +
                             input.ny * instance.transform0[1] +
                             input.nz * instance.transform0[2];
            const float ny = input.nx * instance.transform1[0] +
                             input.ny * instance.transform1[1] +
                             input.nz * instance.transform1[2];
            const float nz = input.nx * instance.transform2[0] +
                             input.ny * instance.transform2[1] +
                             input.nz * instance.transform2[2];

            std::array<float, 4> color = instance.baseColor;
            if (lighting)
            {
                const float dot =
                    nx * constants.bmdLightPosition[0] +
                    ny * constants.bmdLightPosition[1] +
                    nz * constants.bmdLightPosition[2];
                const float factor = std::max(dot * 0.8f + 0.4f, 0.2f);
                color = {
                    instance.bodyLightAndAlpha[0] * factor,
                    instance.bodyLightAndAlpha[1] * factor,
                    instance.bodyLightAndAlpha[2] * factor,
                    instance.bodyLightAndAlpha[3],
                };
            }

            float u = input.u;
            float v = input.v;
            if (uvAnimation)
            {
                u += instance.uvAnimation[0];
                v += instance.uvAnimation[1];
            }

            expanded.push_back({
                x, y, z, nx, ny, nz, u, v, PackColor(color)
            });
        }
    }

    return SubmitTriangles(expanded, geometry.TextureId());
}

bool LegacyRenderFacade::MatrixMode(LegacyMatrixMode mode) noexcept { mu::GetRenderer().SetMatrixMode(static_cast<int>(mode)); return true; }
bool LegacyRenderFacade::LoadIdentity() noexcept { mu::GetRenderer().LoadIdentity(); return true; }
bool LegacyRenderFacade::LoadMatrix(const std::array<float, 16>& matrix) noexcept { mu::GetRenderer().LoadMatrix(matrix.data()); return true; }
bool LegacyRenderFacade::MultMatrix(const std::array<float, 16>& matrix) noexcept { mu::GetRenderer().MultMatrix(matrix.data()); return true; }
bool LegacyRenderFacade::Translate(float x, float y, float z) noexcept { mu::GetRenderer().Translate(x, y, z); return true; }
bool LegacyRenderFacade::Scale(float x, float y, float z) noexcept { mu::GetRenderer().Scale(x, y, z); return true; }
bool LegacyRenderFacade::Rotate(float angle, float x, float y, float z) noexcept { mu::GetRenderer().Rotate(angle, x, y, z); return true; }

bool LegacyRenderFacade::Ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane) noexcept
{
    const glm::mat4 matrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    mu::GetRenderer().MultMatrix(glm::value_ptr(matrix));
    return true;
}

bool LegacyRenderFacade::Perspective(float fovY, float aspect, float nearPlane, float farPlane) noexcept
{
    if (aspect == 0.0f || nearPlane <= 0.0f || farPlane <= nearPlane)
        return false;
    const glm::mat4 matrix = glm::perspective(glm::radians(fovY), aspect, nearPlane, farPlane);
    mu::GetRenderer().MultMatrix(glm::value_ptr(matrix));
    return true;
}

bool LegacyRenderFacade::PushMatrix() noexcept { mu::GetRenderer().PushMatrix(); return true; }
bool LegacyRenderFacade::PopMatrix() noexcept { mu::GetRenderer().PopMatrix(); return true; }

bool LegacyRenderFacade::Sphere(float radius, unsigned int slices, unsigned int stacks) noexcept
{
    if (!(radius > 0.0f) || slices < 3 || stacks < 2)
        return false;

    constexpr float pi = 3.14159265358979323846f;
    std::vector<RenderTapeVertex> triangles;
    triangles.reserve(static_cast<std::size_t>(slices) *
                      static_cast<std::size_t>(stacks) * 6u);

    const auto makeVertex = [radius, pi](float theta, float phi) {
        RenderTapeVertex vertex{};
        const float sinPhi = std::sin(phi);
        const float nx = std::cos(theta) * sinPhi;
        const float ny = std::sin(theta) * sinPhi;
        const float nz = std::cos(phi);
        vertex.position = {nx * radius, ny * radius, nz * radius};
        vertex.normal = {nx, ny, nz};
        vertex.texCoord = {theta / (2.0f * pi), phi / pi};
        return vertex;
    };

    for (unsigned int stack = 0; stack < stacks; ++stack)
    {
        const float phi0 = pi * static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi1 = pi * static_cast<float>(stack + 1u) / static_cast<float>(stacks);

        for (unsigned int slice = 0; slice < slices; ++slice)
        {
            const float theta0 = 2.0f * pi * static_cast<float>(slice) / static_cast<float>(slices);
            const float theta1 = 2.0f * pi * static_cast<float>(slice + 1u) / static_cast<float>(slices);

            const auto a = makeVertex(theta0, phi0);
            const auto b = makeVertex(theta1, phi0);
            const auto c = makeVertex(theta1, phi1);
            const auto d = makeVertex(theta0, phi1);

            if (stack != 0)
            {
                triangles.push_back(a);
                triangles.push_back(b);
                triangles.push_back(c);
            }
            if (stack + 1u != stacks)
            {
                triangles.push_back(a);
                triangles.push_back(c);
                triangles.push_back(d);
            }
        }
    }

    return EmitPrimitiveDraw(LegacyPrimitive::Triangles, triangles);
}

bool LegacyRenderFacade::ApplyMatrix(const std::array<float, 16>& matrix) noexcept { return MultMatrix(matrix); }

bool LegacyRenderFacade::PushAttrib() noexcept
{
    m_attribStack.push_back(m_state);
    return true;
}

bool LegacyRenderFacade::PopAttrib() noexcept
{
    if (m_attribStack.empty())
        return false;
    m_state = m_attribStack.back();
    m_attribStack.pop_back();
    return true;
}

bool LegacyRenderFacade::PushClientAttrib() noexcept
{
    m_clientAttribStack.push_back({m_vertexArray, m_normalArray, m_colorArray, m_texCoordArray});
    return true;
}

bool LegacyRenderFacade::PopClientAttrib() noexcept
{
    if (m_clientAttribStack.empty())
        return false;
    auto state = std::move(m_clientAttribStack.back());
    m_clientAttribStack.pop_back();
    m_vertexArray = std::move(state.vertex);
    m_normalArray = std::move(state.normal);
    m_colorArray = std::move(state.color);
    m_texCoordArray = std::move(state.texCoord);
    return true;
}

bool LegacyRenderFacade::SetOpaqueState() noexcept { m_state.blendEnabled = false; m_state.alphaTestEnabled = false; mu::GetRenderer().DisableBlend(); mu::GetRenderer().SetAlphaTest(false); return true; }
bool LegacyRenderFacade::SetAlphaTestState(bool enabled) noexcept { return SetAlphaTestEnable(enabled); }
bool LegacyRenderFacade::SetAdditiveState() noexcept { m_state.blendEnabled = true; m_state.blendSource = RenderBlendFactor::SourceAlpha; m_state.blendDestination = RenderBlendFactor::One; mu::GetRenderer().SetBlendMode(mu::BlendMode::Additive); return true; }
bool LegacyRenderFacade::SetTextureEnable(bool enabled) noexcept { m_state.textureEnabled = enabled; mu::GetRenderer().SetTexture2D(enabled); return true; }
bool LegacyRenderFacade::SetDepthTestEnable(bool enabled) noexcept { m_state.depthTestEnabled = enabled; mu::GetRenderer().SetDepthTest(enabled); return true; }
bool LegacyRenderFacade::SetDepthWriteEnable(bool enabled) noexcept { m_state.depthWriteEnabled = enabled; mu::GetRenderer().SetDepthMask(enabled); return true; }
bool LegacyRenderFacade::SetDepthFunc(RenderCompareFunction func) noexcept { m_state.depthFunc = func; mu::GetRenderer().SetDepthFunc(static_cast<int>(func)); return true; }
bool LegacyRenderFacade::SetCullEnable(bool enabled) noexcept { m_state.cullEnabled = enabled; mu::GetRenderer().SetCullFace(enabled); return true; }
bool LegacyRenderFacade::SetCullFace(RenderCullFace face) noexcept { m_state.cullFace = face; return true; }
bool LegacyRenderFacade::SetFrontFace(RenderFrontFace face) noexcept { m_state.frontFace = face; mu::GetRenderer().SetFrontFace(static_cast<int>(face)); return true; }
bool LegacyRenderFacade::SetBlendEnable(bool enabled) noexcept { m_state.blendEnabled = enabled; if (enabled) mu::GetRenderer().SetBlendMode(mu::BlendMode::Alpha); else mu::GetRenderer().DisableBlend(); return true; }

bool LegacyRenderFacade::SetBlendFunc(RenderBlendFactor source, RenderBlendFactor destination) noexcept
{
    m_state.blendSource = source;
    m_state.blendDestination = destination;
    m_state.blendEnabled = true;
    if (source == RenderBlendFactor::SourceAlpha && destination == RenderBlendFactor::OneMinusSourceAlpha)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Alpha);
    else if (source == RenderBlendFactor::SourceAlpha && destination == RenderBlendFactor::One)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Additive);
    else if (source == RenderBlendFactor::Zero && destination == RenderBlendFactor::OneMinusSourceColor)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Subtract);
    else if (source == RenderBlendFactor::OneMinusDestinationColor && destination == RenderBlendFactor::Zero)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::InverseColor);
    else if (source == RenderBlendFactor::One && destination == RenderBlendFactor::OneMinusSourceAlpha)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Mixed);
    else if (source == RenderBlendFactor::Zero && destination == RenderBlendFactor::SourceColor)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::LightMap);
    else if (source == RenderBlendFactor::One && destination == RenderBlendFactor::One)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Glow);
    else if (source == RenderBlendFactor::OneMinusSourceColor && destination == RenderBlendFactor::One)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Luminance);
    else
        return false;
    return true;
}

bool LegacyRenderFacade::SetAlphaTestEnable(bool enabled) noexcept { m_state.alphaTestEnabled = enabled; mu::GetRenderer().SetAlphaTest(enabled); return true; }
bool LegacyRenderFacade::SetAlphaFunc(RenderCompareFunction func, float ref) noexcept { m_state.alphaFunc = func; m_state.alphaRef = ref; mu::GetRenderer().SetAlphaFunc(static_cast<int>(func), ref); return true; }
bool LegacyRenderFacade::SetFogEnable(bool enabled) noexcept { m_state.fogEnabled = enabled; mu::GetRenderer().SetFogEnabled(enabled); return true; }

bool LegacyRenderFacade::RefreshFog() noexcept
{
    mu::FogParams params{};
    params.mode = m_fogMode;
    params.start = m_fogStart;
    params.end = m_fogEnd;
    params.density = m_fogDensity;
    std::copy(m_fogColor.begin(), m_fogColor.end(), params.color);
    mu::GetRenderer().SetFog(params);
    return true;
}

bool LegacyRenderFacade::SetFogMode(RenderFogMode mode) noexcept { m_fogMode = static_cast<int>(mode); m_state.fogMode = m_fogMode; return RefreshFog(); }
bool LegacyRenderFacade::SetFogColor(const std::array<float, 4>& color) noexcept { m_fogColor = color; m_state.fogColor = color; return RefreshFog(); }
bool LegacyRenderFacade::SetFogRange(float start, float end) noexcept { m_fogStart = start; m_fogEnd = end; m_state.fogStart = start; m_state.fogEnd = end; return RefreshFog(); }
bool LegacyRenderFacade::SetFogDensity(float density) noexcept { m_fogDensity = density; m_state.fogDensity = density; return RefreshFog(); }
bool LegacyRenderFacade::SetLightingEnable(bool enabled) noexcept { m_state.lightingEnabled = enabled; return true; }
bool LegacyRenderFacade::SetColorMask(bool r, bool g, bool b, bool a) noexcept { m_state.colorMaskR = r; m_state.colorMaskG = g; m_state.colorMaskB = b; m_state.colorMaskA = a; mu::GetRenderer().SetColorMask(r, g, b, a); return true; }
bool LegacyRenderFacade::SetStencilEnable(bool enabled) noexcept { m_state.stencilEnabled = enabled; mu::GetRenderer().SetStencilTest(enabled); return true; }
bool LegacyRenderFacade::SetStencilFunc(RenderCompareFunction func, unsigned int ref, unsigned int mask) noexcept { m_state.stencilFunc = func; m_state.stencilRef = ref; m_state.stencilMask = mask; mu::GetRenderer().SetStencilFunc(static_cast<int>(func), static_cast<int>(ref), mask); return true; }
bool LegacyRenderFacade::SetStencilOp(RenderStencilOperation fail, RenderStencilOperation depthFail, RenderStencilOperation pass) noexcept { m_state.stencilFail = fail; m_state.stencilDepthFail = depthFail; m_state.stencilPass = pass; mu::GetRenderer().SetStencilOp(static_cast<int>(fail), static_cast<int>(depthFail), static_cast<int>(pass)); return true; }
bool LegacyRenderFacade::SetTextureEnvironment(RenderTextureEnvironment environment) noexcept { m_state.textureEnvironment = environment; mu::GetRenderer().SetTexEnv(0x2300, 0x2200, static_cast<int>(environment)); return true; }
bool LegacyRenderFacade::SetShadeMode(RenderShadeMode mode) noexcept { m_state.shadeMode = mode; return true; }
bool LegacyRenderFacade::SetLineWidth(float width) noexcept { if (!(width > 0.0f)) return false; m_state.lineWidth = width; return true; }
bool LegacyRenderFacade::SetPolygonMode(RenderCullFace face, RenderPolygonMode mode) noexcept { m_state.cullFace = face; m_state.polygonMode = mode; mu::GetRenderer().SetPolygonMode(static_cast<int>(face), static_cast<int>(mode)); return true; }
bool LegacyRenderFacade::SetViewport(RenderTapeRect rect) noexcept { m_state.viewport = rect; mu::GetRenderer().SetViewport(rect.x, rect.y, rect.width, rect.height); return true; }
bool LegacyRenderFacade::SetScissorEnable(bool enabled) noexcept { m_state.scissorEnabled = enabled; mu::GetRenderer().SetScissorEnabled(enabled); return true; }
bool LegacyRenderFacade::SetScissor(RenderTapeRect rect) noexcept { m_state.scissor = rect; mu::GetRenderer().SetScissor(rect.x, rect.y, rect.width, rect.height); return true; }
bool LegacyRenderFacade::SetClearColor(const std::array<float, 4>& color) noexcept { m_state.clearColor = color; mu::GetRenderer().SetClearColor(color[0], color[1], color[2], color[3]); return true; }
bool LegacyRenderFacade::SetClearDepth(float depth) noexcept { m_state.clearDepth = depth; return depth >= 0.0f && depth <= 1.0f; }
bool LegacyRenderFacade::SetClearStencilValue(unsigned int value) noexcept { m_state.clearStencil = value; return true; }

bool LegacyRenderFacade::Clear(bool color, bool depth, bool stencil) noexcept
{
    if (m_recording.IsRecording())
        return m_recording.AppendClear(color, depth, stencil, m_state);

    if (color)
        mu::GetRenderer().ClearScreen();
    else if (depth)
        mu::GetRenderer().ClearDepthBuffer();
    return true;
}

bool LegacyRenderFacade::ClearStencil(unsigned int value) noexcept
{
    m_state.clearStencil = value;
    return Clear(false, false, true);
}

bool LegacyRenderFacade::DefineTexture2D(LogicalRenderAssetRef ref, unsigned int width, unsigned int height,
                                         std::span<const std::byte> pixels, LegacyPixelFormat format,
                                         RenderAssetRetention retention, RenderSamplerIntent sampler) noexcept
{
    return m_assets.DefineTexture2D(ref, width, height, pixels, format, retention, sampler);
}

bool LegacyRenderFacade::AppendFrameOnlyTextureQuad(
    LogicalRenderAssetRef ref, unsigned int width, unsigned int height,
    std::span<const std::byte> pixels, RenderSamplerIntent sampler,
    const std::array<std::array<float, 2>, 4>& positions) noexcept
{
    if (!DefineTexture2D(ref, width, height, pixels, LegacyPixelFormat::Rgba8,
                         RenderAssetRetention::FrameOnly, sampler))
        return false;

    const auto metadata = m_assets.Resolve(ref);
    if (!metadata || metadata->textureId == 0)
        return false;

    const mu::Vertex2D vertices[4] = {
        {positions[0][0], positions[0][1], 0.0f, 0.0f, 0xFFFFFFFFu},
        {positions[1][0], positions[1][1], 0.0f, 1.0f, 0xFFFFFFFFu},
        {positions[2][0], positions[2][1], 1.0f, 1.0f, 0xFFFFFFFFu},
        {positions[3][0], positions[3][1], 1.0f, 0.0f, 0xFFFFFFFFu},
    };
    return SubmitQuad2D(vertices, metadata->textureId);
}

void LegacyRenderFacade::BindTexture(LogicalRenderAssetRef ref) noexcept
{
    const auto metadata = m_assets.Resolve(ref);
    m_boundTextureId = metadata ? metadata->textureId : 0;
    mu::GetRenderer().BindTexture(static_cast<int>(m_boundTextureId));
}

void LegacyRenderFacade::BindTextureId(std::uint32_t textureId) noexcept
{
    m_boundTextureId = textureId;
    mu::GetRenderer().BindTexture(static_cast<int>(m_boundTextureId));
}

LegacyRenderFacade& GetLegacyRenderFacade()
{
    static LegacyRenderFacade facade(GetLogicalRenderAssetTable());
    return facade;
}

} // namespace mu::pipeline
