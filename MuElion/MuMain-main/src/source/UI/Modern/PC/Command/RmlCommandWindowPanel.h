#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Command
{

class RmlCommandWindowPanel
{
public:
    static constexpr std::size_t CommandCount = 11;

    enum class ActionType
    {
        SelectCommand,
        Close,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t commandIndex = 0;
    };

    struct Command
    {
        std::string label;
        bool visible = true;
        bool enabled = true;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        bool applyPosition = false;
        float positionX = 0.0f;
        float positionY = 0.0f;

        std::string title = "Command";
        std::array<Command, CommandCount> commands{};
        std::optional<std::size_t> selectedCommand;
    };

    RmlCommandWindowPanel();
    ~RmlCommandWindowPanel();

    RmlCommandWindowPanel(const RmlCommandWindowPanel&) = delete;
    RmlCommandWindowPanel& operator=(const RmlCommandWindowPanel&) = delete;
    RmlCommandWindowPanel(RmlCommandWindowPanel&&) noexcept;
    RmlCommandWindowPanel& operator=(RmlCommandWindowPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();

    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Command
