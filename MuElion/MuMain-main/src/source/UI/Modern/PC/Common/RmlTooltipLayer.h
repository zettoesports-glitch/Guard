#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace UI::Modern::PC::Common
{

class RmlTooltipLayer
{
public:
    enum class TextColor : std::uint8_t
    {
        Default = 0,
        Blue = 1,
        Gray = 2,
        Red = 3,
        Yellow = 4,
        Green = 5,
        Purple = 6,
        RedPurple = 7,
        Violet = 8,
        Orange = 9,
    };

    enum class Background : std::uint8_t
    {
        None = 0,
        DarkRed = 1,
        DarkBlue = 2,
        DarkYellow = 3,
        GreenBlue = 4,
    };

    enum class Alignment : std::uint8_t
    {
        Left = 0,
        Center = 1,
        Right = 2,
    };

    struct Line
    {
        std::string text;
        TextColor color = TextColor::Default;
        Background background = Background::None;
        bool bold = false;
    };

    struct Tooltip
    {
        bool visible = false;
        float x = 0.0f;
        float y = 0.0f;
        float minimumWidth = 0.0f;
        Alignment alignment = Alignment::Center;
        bool framed = false;
        std::vector<Line> lines;
    };

    RmlTooltipLayer();
    ~RmlTooltipLayer();

    RmlTooltipLayer(const RmlTooltipLayer&) = delete;
    RmlTooltipLayer& operator=(const RmlTooltipLayer&) = delete;
    RmlTooltipLayer(RmlTooltipLayer&&) noexcept;
    RmlTooltipLayer& operator=(RmlTooltipLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = true);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] std::size_t Capacity() const noexcept;

    [[nodiscard]] bool SetTooltip(
        std::size_t index, const Tooltip& tooltip);
    [[nodiscard]] bool HideTooltip(std::size_t index);
    void HideAll();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Common
