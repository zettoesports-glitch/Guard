#include "stdafx.h"
#include "client/render/LegacyRenderFacade.h"

#include "Render/Renderer/MuRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
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

bool LegacyRenderFacade::EmitPrimitiveDraw(LegacyPrimitive primitive, std::span<const RenderTapeVertex> vertices) noexcept
{
    if (vertices.empty())
        return true;

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
    case LegacyPrimitive::TriangleFan: {
        if (vertices.size() < 3) return true;
        std::vector<RenderTapeVertex> expanded;
        expanded.reserve((vertices.size() - 2) * 3);
        for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
        {
            expanded.push_back(vertices[0]);
            expanded.push_back(vertices[i]);
            expanded.push_back(vertices[i + 1]);
        }
        return EmitPrimitiveDraw(LegacyPrimitive::Triangles, expanded);
    }
    case LegacyPrimitive::TriangleStrip: {
        if (vertices.size() < 3) return true;
        std::vector<RenderTapeVertex> expanded;
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
    case LegacyPrimitive::LineStrip:
    case LegacyPrimitive::LineLoop: {
        if (vertices.size() < 2) return true;
        std::vector<RenderTapeVertex> expanded;
        const std::size_t pairCount = primitive == LegacyPrimitive::LineLoop ? vertices.size() : vertices.size() - 1;
        expanded.reserve(pairCount * 2);
        for (std::size_t i = 0; i < pairCount; ++i)
        {
            expanded.push_back(vertices[i]);
            expanded.push_back(vertices[(i + 1) % vertices.size()]);
        }
        return EmitPrimitiveDraw(LegacyPrimitive::Lines, expanded);
    }
    default:
        return false;
    }
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

bool LegacyRenderFacade::DrawArrays(LegacyPrimitive primitive, int first, int count) noexcept
{
    if (first < 0 || count < 0)
        return false;

    std::vector<RenderTapeVertex> vertices(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
        if (!ReadArrayVertex(first + i, vertices[static_cast<std::size_t>(i)])) return false;

    return EmitPrimitiveDraw(primitive, vertices);
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

bool LegacyRenderFacade::SetOpaqueState() noexcept { mu::GetRenderer().DisableBlend(); mu::GetRenderer().SetAlphaTest(false); return true; }
bool LegacyRenderFacade::SetAlphaTestState(bool enabled) noexcept { return SetAlphaTestEnable(enabled); }
bool LegacyRenderFacade::SetAdditiveState() noexcept { mu::GetRenderer().SetBlendMode(mu::BlendMode::Additive); return true; }
bool LegacyRenderFacade::SetTextureEnable(bool enabled) noexcept { mu::GetRenderer().SetTexture2D(enabled); return true; }
bool LegacyRenderFacade::SetDepthTestEnable(bool enabled) noexcept { mu::GetRenderer().SetDepthTest(enabled); return true; }
bool LegacyRenderFacade::SetDepthWriteEnable(bool enabled) noexcept { mu::GetRenderer().SetDepthMask(enabled); return true; }
bool LegacyRenderFacade::SetDepthFunc(RenderCompareFunction func) noexcept { mu::GetRenderer().SetDepthFunc(static_cast<int>(func)); return true; }
bool LegacyRenderFacade::SetCullEnable(bool enabled) noexcept { mu::GetRenderer().SetCullFace(enabled); return true; }
bool LegacyRenderFacade::SetCullFace(RenderCullFace) noexcept { return true; }
bool LegacyRenderFacade::SetFrontFace(RenderFrontFace face) noexcept { mu::GetRenderer().SetFrontFace(static_cast<int>(face)); return true; }
bool LegacyRenderFacade::SetBlendEnable(bool enabled) noexcept { if (enabled) mu::GetRenderer().SetBlendMode(mu::BlendMode::Alpha); else mu::GetRenderer().DisableBlend(); return true; }

bool LegacyRenderFacade::SetBlendFunc(RenderBlendFactor source, RenderBlendFactor destination) noexcept
{
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
    else
        return false;
    return true;
}

bool LegacyRenderFacade::SetAlphaTestEnable(bool enabled) noexcept { mu::GetRenderer().SetAlphaTest(enabled); return true; }
bool LegacyRenderFacade::SetAlphaFunc(RenderCompareFunction func, float ref) noexcept { mu::GetRenderer().SetAlphaFunc(static_cast<int>(func), ref); return true; }
bool LegacyRenderFacade::SetFogEnable(bool enabled) noexcept { mu::GetRenderer().SetFogEnabled(enabled); return true; }

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

bool LegacyRenderFacade::SetFogMode(RenderFogMode mode) noexcept { m_fogMode = static_cast<int>(mode); return RefreshFog(); }
bool LegacyRenderFacade::SetFogColor(const std::array<float, 4>& color) noexcept { m_fogColor = color; return RefreshFog(); }
bool LegacyRenderFacade::SetFogRange(float start, float end) noexcept { m_fogStart = start; m_fogEnd = end; return RefreshFog(); }
bool LegacyRenderFacade::SetFogDensity(float density) noexcept { m_fogDensity = density; return RefreshFog(); }
bool LegacyRenderFacade::SetColorMask(bool r, bool g, bool b, bool a) noexcept { mu::GetRenderer().SetColorMask(r, g, b, a); return true; }
bool LegacyRenderFacade::SetStencilEnable(bool enabled) noexcept { mu::GetRenderer().SetStencilTest(enabled); return true; }
bool LegacyRenderFacade::SetStencilFunc(RenderCompareFunction func, unsigned int ref, unsigned int mask) noexcept { mu::GetRenderer().SetStencilFunc(static_cast<int>(func), static_cast<int>(ref), mask); return true; }
bool LegacyRenderFacade::SetStencilOp(RenderStencilOperation fail, RenderStencilOperation depthFail, RenderStencilOperation pass) noexcept { mu::GetRenderer().SetStencilOp(static_cast<int>(fail), static_cast<int>(depthFail), static_cast<int>(pass)); return true; }
bool LegacyRenderFacade::SetTextureEnvironment(RenderTextureEnvironment environment) noexcept { mu::GetRenderer().SetTexEnv(0x2300, 0x2200, static_cast<int>(environment)); return true; }
bool LegacyRenderFacade::SetPolygonMode(RenderCullFace face, RenderPolygonMode mode) noexcept { mu::GetRenderer().SetPolygonMode(static_cast<int>(face), static_cast<int>(mode)); return true; }
bool LegacyRenderFacade::SetViewport(RenderTapeRect rect) noexcept { mu::GetRenderer().SetViewport(rect.x, rect.y, rect.width, rect.height); return true; }
bool LegacyRenderFacade::SetScissorEnable(bool enabled) noexcept { mu::GetRenderer().SetScissorEnabled(enabled); return true; }
bool LegacyRenderFacade::SetScissor(RenderTapeRect rect) noexcept { mu::GetRenderer().SetScissor(rect.x, rect.y, rect.width, rect.height); return true; }
bool LegacyRenderFacade::SetClearColor(const std::array<float, 4>& color) noexcept { mu::GetRenderer().SetClearColor(color[0], color[1], color[2], color[3]); return true; }

bool LegacyRenderFacade::Clear(bool color, bool depth, bool) noexcept
{
    if (color)
        mu::GetRenderer().ClearScreen();
    else if (depth)
        mu::GetRenderer().ClearDepthBuffer();
    return true;
}

bool LegacyRenderFacade::DefineTexture2D(LogicalRenderAssetRef ref, unsigned int width, unsigned int height,
                                         std::span<const std::byte> pixels, LegacyPixelFormat format,
                                         RenderAssetRetention retention, RenderSamplerIntent sampler) noexcept
{
    return m_assets.DefineTexture2D(ref, width, height, pixels, format, retention, sampler);
}

void LegacyRenderFacade::BindTexture(LogicalRenderAssetRef ref) noexcept
{
    const auto metadata = m_assets.Resolve(ref);
    m_boundTextureId = metadata ? metadata->textureId : 0;
    mu::GetRenderer().BindTexture(static_cast<int>(m_boundTextureId));
}

LegacyRenderFacade& GetLegacyRenderFacade()
{
    static LegacyRenderFacade facade(GetLogicalRenderAssetTable());
    return facade;
}

} // namespace mu::pipeline
