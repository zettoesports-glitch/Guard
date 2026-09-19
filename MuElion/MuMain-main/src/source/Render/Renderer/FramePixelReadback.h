#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace mu
{

struct FramePixels
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgb;
};

struct FramePixelsRgba8
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

enum class PixelChannelOrder
{
    Rgba,
    Bgra,
};

[[nodiscard]] bool ConvertToTopDownRgb(std::span<const std::uint8_t> source,
                                       std::uint32_t width,
                                       std::uint32_t height,
                                       std::uint32_t rowPitch,
                                       PixelChannelOrder channelOrder,
                                       bool reverseRows,
                                       FramePixels& output);

[[nodiscard]] bool ConvertToTopDownRgba8(std::span<const std::uint8_t> source,
                                         std::uint32_t width,
                                         std::uint32_t height,
                                         std::uint32_t rowPitch,
                                         PixelChannelOrder channelOrder,
                                         bool reverseRows,
                                         FramePixelsRgba8& output);

class FrameReadbackState
{
public:
    [[nodiscard]] bool Request();
    [[nodiscard]] bool IsPending() const;
    void Complete(FramePixels pixels);
    void Fail();
    [[nodiscard]] FramePixels Consume();
    void Reset();

private:
    enum class State : std::uint8_t
    {
        Idle,
        Pending,
        Completed,
    };

    State state_ = State::Idle;
    FramePixels pixels_;
};

class FrameRgbaReadbackState
{
public:
    [[nodiscard]] bool Request(bool reverseRows);
    [[nodiscard]] bool IsPending() const;
    [[nodiscard]] bool ReverseRows() const;
    void Complete(FramePixelsRgba8 pixels);
    void Fail();
    [[nodiscard]] FramePixelsRgba8 Consume();
    void Reset();

private:
    enum class State : std::uint8_t
    {
        Idle,
        Pending,
        Completed,
    };

    State state_ = State::Idle;
    bool reverseRows_ = false;
    FramePixelsRgba8 pixels_;
};

} // namespace mu
