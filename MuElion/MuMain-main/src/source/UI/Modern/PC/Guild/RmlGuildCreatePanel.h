#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Guild
{

// Semantic reconstruction name. The Debug executable preserves the document
// contract but no RmlGuildCreatePanel RTTI name.
class RmlGuildCreatePanel
{
public:
    static constexpr std::size_t MarkCellCount = 64;
    static constexpr std::size_t ColorCount = 16;

    enum class Page : std::uint8_t
    {
        Intro,
        Mark,
        Confirm,
    };

    enum class ActionType : std::uint8_t
    {
        Close,
        Previous,
        Next,
        Confirm,
        SelectColor,
        PaintCell,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
        std::uint8_t value = 0;
        std::string guildName;
        std::array<std::uint8_t, MarkCellCount> mark{};
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        float x = 20.0f;
        float y = 20.0f;
        Page page = Page::Intro;
        std::string title = "Guild";
        std::string guildName;
        std::string guildMaster;
        std::array<std::uint8_t, MarkCellCount> mark{};
        std::uint8_t selectedColor = 0;
    };

    RmlGuildCreatePanel();
    ~RmlGuildCreatePanel();

    RmlGuildCreatePanel(const RmlGuildCreatePanel&) = delete;
    RmlGuildCreatePanel& operator=(const RmlGuildCreatePanel&) = delete;
    RmlGuildCreatePanel(RmlGuildCreatePanel&&) noexcept;
    RmlGuildCreatePanel& operator=(RmlGuildCreatePanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<Action> ConsumeAction();
    [[nodiscard]] const State& GetState() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Guild
