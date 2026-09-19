#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mu::pipeline
{

enum class LegacyPrimitive : std::uint32_t
{
    Points = 0x0000,
    Lines = 0x0001,
    LineLoop = 0x0002,
    LineStrip = 0x0003,
    Triangles = 0x0004,
    TriangleStrip = 0x0005,
    TriangleFan = 0x0006,
    Quads = 0x0007,
    QuadStrip = 0x0008,
};

enum class LegacyMatrixMode : std::uint32_t
{
    ModelView = 0x1700,
    Projection = 0x1701,
    Texture = 0x1702,
};

enum class RenderCompareFunction : std::uint32_t
{
    Never = 0x0200,
    Less = 0x0201,
    Equal = 0x0202,
    LessEqual = 0x0203,
    Greater = 0x0204,
    NotEqual = 0x0205,
    GreaterEqual = 0x0206,
    Always = 0x0207,
};

enum class RenderCullFace : std::uint32_t
{
    Front = 0x0404,
    Back = 0x0405,
    FrontAndBack = 0x0408,
};

enum class RenderFrontFace : std::uint32_t
{
    Clockwise = 0x0900,
    CounterClockwise = 0x0901,
};

enum class RenderBlendFactor : std::uint32_t
{
    Zero = 0,
    One = 1,
    SourceColor = 0x0300,
    OneMinusSourceColor = 0x0301,
    SourceAlpha = 0x0302,
    OneMinusSourceAlpha = 0x0303,
    DestinationAlpha = 0x0304,
    OneMinusDestinationAlpha = 0x0305,
    DestinationColor = 0x0306,
    OneMinusDestinationColor = 0x0307,
};

enum class RenderFogMode : std::uint32_t
{
    Exp = 0x0800,
    Exp2 = 0x0801,
    Linear = 0x2601,
};

enum class RenderStencilOperation : std::uint32_t
{
    Zero = 0,
    Keep = 0x1E00,
    Replace = 0x1E01,
    Increment = 0x1E02,
    Decrement = 0x1E03,
    Invert = 0x150A,
};

enum class RenderTextureEnvironment : std::uint32_t
{
    Modulate = 0x2100,
    Decal = 0x2101,
    Blend = 0x0BE2,
    Replace = 0x1E01,
    Add = 0x0104,
};

enum class RenderShadeMode : std::uint32_t
{
    Flat = 0x1D00,
    Smooth = 0x1D01,
};

enum class RenderPolygonMode : std::uint32_t
{
    Point = 0x1B00,
    Line = 0x1B01,
    Fill = 0x1B02,
};

enum class RenderClientArraySemantic : std::uint32_t
{
    Vertex = 0x8074,
    Normal = 0x8075,
    Color = 0x8076,
    TextureCoordinate = 0x8078,
};

enum class RenderClientArrayScalarType : std::uint32_t
{
    Byte = 0x1400,
    UnsignedByte = 0x1401,
    Short = 0x1402,
    UnsignedShort = 0x1403,
    Int = 0x1404,
    UnsignedInt = 0x1405,
    Float = 0x1406,
    Double = 0x140A,
};

enum class LegacyPixelFormat : std::uint8_t
{
    Rgb8,
    Rgba8,
    Bgra8,
};

enum class RenderAssetRetention : std::uint8_t
{
    FrameOnly,
    Shared,
    Persistent,
};

struct RenderSamplerIntent
{
    bool linear = true;
    bool repeatU = false;
    bool repeatV = false;
};

struct RenderTapeRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct LogicalRenderAssetRef
{
    std::uint64_t id = 0;
    std::uint64_t revision = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return id != 0; }
    friend constexpr bool operator==(const LogicalRenderAssetRef&, const LogicalRenderAssetRef&) = default;
};

struct RenderTapeVertex
{
    std::array<float, 3> position{};
    std::array<float, 3> normal{0.0f, 0.0f, 1.0f};
    std::array<float, 2> texCoord{};
    std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

} // namespace mu::pipeline
