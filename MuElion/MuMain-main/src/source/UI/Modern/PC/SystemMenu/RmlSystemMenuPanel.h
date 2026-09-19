#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::SystemMenu
{

class RmlSystemMenuPanel
{
public:
    enum class ActionType
    {
        Exit,
        Server,
        Character,
        Option,
        Close,
    };

    struct ButtonState
    {
        bool visible = true;
        bool enabled = true;
        std::string label;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string title = "System";
        ButtonState exit{true, true, "Exit"};
        ButtonState server{true, true, "Server"};
        ButtonState character{true, true, "Character"};
        ButtonState option{true, true, "Option"};
        ButtonState close{true, true, "Close"};
    };

    RmlSystemMenuPanel();
    ~RmlSystemMenuPanel();

    RmlSystemMenuPanel(const RmlSystemMenuPanel&) = delete;
    RmlSystemMenuPanel& operator=(const RmlSystemMenuPanel&) = delete;
    RmlSystemMenuPanel(RmlSystemMenuPanel&&) noexcept;
    RmlSystemMenuPanel& operator=(RmlSystemMenuPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();

    [[nodiscard]] std::optional<ActionType> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::SystemMenu
