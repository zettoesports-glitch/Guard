#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Common
{

class RmlMessageBoxPanel
{
public:
    enum class InputKind
    {
        None,
        Text,
        Number,
        Password,
    };

    enum class ActionType
    {
        Ok,
        Cancel,
    };

    struct Action
    {
        ActionType type = ActionType::Cancel;
        std::string input;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        bool cautionStyle = false;
        bool twoButtons = false;
        bool messageOnly = false;

        std::string title;
        std::vector<std::string> lines;

        InputKind inputKind = InputKind::None;
        std::string input;
        std::string okLabel = "OK";
        std::string cancelLabel = "Cancel";

        bool okEnabled = true;
        bool cancelEnabled = true;
        bool enterAccepts = true;
        bool escapeCancels = true;
    };

    RmlMessageBoxPanel();
    ~RmlMessageBoxPanel();

    RmlMessageBoxPanel(const RmlMessageBoxPanel&) = delete;
    RmlMessageBoxPanel& operator=(const RmlMessageBoxPanel&) = delete;
    RmlMessageBoxPanel(RmlMessageBoxPanel&&) noexcept;
    RmlMessageBoxPanel& operator=(RmlMessageBoxPanel&&) noexcept;

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

} // namespace UI::Modern::PC::Common
