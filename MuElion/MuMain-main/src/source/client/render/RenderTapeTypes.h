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

// GPU layouts recovered directly from the embedded MuTwo vertex shader.
// These types deliberately preserve the raw float4/uint4 layout while higher
// level semantic names continue to be reconstructed from the Debug executable.
using RenderTapeFloat4 = std::array<float, 4>;
using RenderTapeUInt4 = std::array<std::uint32_t, 4>;
using RenderTapeMatrix4 = std::array<float, 16>;

struct alignas(16) RenderTapeBoneMatrix
{
    RenderTapeFloat4 row0{};
    RenderTapeFloat4 row1{};
    RenderTapeFloat4 row2{};
};

struct alignas(16) RenderTapeTerrainCell
{
    float height = 0.0f;
    std::uint32_t wall = 0;
    float alpha = 1.0f;
    float padding = 0.0f;
};

// This maps the non-matrix part of the shader's
// type_RenderTapeVertexConstants block. The grouping under the private
// RenderTapeBmdConstants C++ type is reconstructed from the recovered facade
// signature; field order itself is confirmed by the embedded shader.
struct alignas(8) RenderTapeBmdConstants
{
    // First 192 bytes: confirmed shader-visible payload.
    RenderTapeFloat4 bmdScale{};
    RenderTapeFloat4 bmdBodyOrigin{};
    RenderTapeFloat4 bmdBodyLight{};
    RenderTapeFloat4 bmdBaseColor{};
    RenderTapeFloat4 bmdLightPosition{};
    RenderTapeFloat4 bmdUvAnimation{};
    RenderTapeFloat4 bmdChromeLight{};
    RenderTapeFloat4 bmdLegacyLight{};
    RenderTapeUInt4 bmdMode{};
    RenderTapeFloat4 rigidTransform0{};
    RenderTapeFloat4 rigidTransform1{};
    RenderTapeFloat4 rigidTransform2{};

    // DrawBmdGeometry/DrawRigidInstances copy 0xE8 bytes from the private
    // argument in Main-x64-Debug.exe. The final 40 bytes are CPU-side
    // metadata not consumed by the embedded vertex shader. Keep them opaque
    // until their individual fields are recovered.
    std::array<std::byte, 40> cpuMetadata{};
};

struct RenderTapeTerrainConstants
{
    // DrawTerrainInstances copies exactly 0x24 bytes from this argument in the
    // x64 Debug binary. Field semantics are still being recovered.
    std::array<std::byte, 36> raw{};
};

struct alignas(16) RenderTapeVertexConstants
{
    RenderTapeMatrix4 modelView{};
    RenderTapeMatrix4 projection{};
    RenderTapeMatrix4 textureMatrix{};
    RenderTapeBmdConstants bmd{};
};

// bmdMode.w bit usage recovered from the shader. Names describe observed
// behavior and are intentionally kept separate from the original private enum.
enum RenderTapeBmdModeFlag : std::uint32_t
{
    RenderTapeBmdTranslate = 1u << 0,
    RenderTapeBmdLighting = 1u << 1,
    RenderTapeBmdUvAnimation = 1u << 2,
    RenderTapeBmdWave = 1u << 3,
    RenderTapeBmdBoneScalePath = 1u << 4,
    RenderTapeBmdTerrainLightOverride = 1u << 5,
    RenderTapeBmdFlag64 = 1u << 6,
};

// Mode 6 in the embedded shader consumes exactly six float4 rows per rigid
// instance. The semantic mapping below is directly observable in the shader.
struct alignas(16) RenderTapeRigidInstance
{
    RenderTapeFloat4 transform0{};
    RenderTapeFloat4 transform1{};
    RenderTapeFloat4 transform2{};
    RenderTapeFloat4 bodyLightAndAlpha{};
    RenderTapeFloat4 baseColor{};
    RenderTapeFloat4 uvAnimation{};
};

static_assert(sizeof(RenderTapeBoneMatrix) == 48);
static_assert(sizeof(RenderTapeTerrainCell) == 16);
static_assert(sizeof(RenderTapeBmdConstants) == 232);
static_assert(sizeof(RenderTapeTerrainConstants) == 36);
static_assert(sizeof(RenderTapeRigidInstance) == 96);

// The GPU constant buffer is the three 4x4 matrices plus only the first
// 192-byte shader-visible prefix of RenderTapeBmdConstants.
static_assert(sizeof(RenderTapeMatrix4) * 3u + 192u == 384u);

} // namespace mu::pipeline
