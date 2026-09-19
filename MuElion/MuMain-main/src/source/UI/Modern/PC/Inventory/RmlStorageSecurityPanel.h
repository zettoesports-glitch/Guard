#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Inventory
{

class RmlStorageSecurityPanel
{
public:
    enum class Mode
    {
        Keypad,
        Password,
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        Mode mode = Mode::Keypad;

        std::string title;
        std::string prompt;
        std::array<int, 10> digitMapping{1,2,3,4,5,6,7,8,9,0};
        int pinLimit = 4;
        bool submitEnabled = true;
    };

    enum class ActionType
    {
        SubmitPin,
        SubmitPassword,
        Cancel,
    };

    struct Action
    {
        ActionType type = ActionType::Cancel;
        std::string value;
    };

    RmlStorageSecurityPanel();
    ~RmlStorageSecurityPanel();

    RmlStorageSecurityPanel(const RmlStorageSecurityPanel&) = delete;
    RmlStorageSecurityPanel& operator=(const RmlStorageSecurityPanel&) = delete;
    RmlStorageSecurityPanel(RmlStorageSecurityPanel&&) noexcept;
    RmlStorageSecurityPanel& operator=(RmlStorageSecurityPanel&&) noexcept;

    [[nodiscard]] bool Load();
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Show(const State& state);
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Inventory
