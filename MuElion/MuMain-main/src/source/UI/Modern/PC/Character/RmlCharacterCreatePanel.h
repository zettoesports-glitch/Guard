#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Character
{

class RmlCharacterCreatePanel
{
public:
    static constexpr std::size_t ClassCount = 7;

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        int selectedClass = 0;
        std::array<bool, ClassCount> classEnabled{};
        std::array<std::string, ClassCount> classNames{};
        std::array<std::string, 5> statNames{};
        std::array<std::string, 5> statValues{};
        int statCount = 4;
        std::string description;
    };

    enum class ActionType : unsigned char
    {
        SelectClass,
        Create,
        Cancel,
    };

    struct Action
    {
        ActionType type = ActionType::Cancel;
        int selectedClass = 0;
        std::string characterName;
    };

    RmlCharacterCreatePanel();
    ~RmlCharacterCreatePanel();

    RmlCharacterCreatePanel(const RmlCharacterCreatePanel&) = delete;
    RmlCharacterCreatePanel& operator=(const RmlCharacterCreatePanel&) = delete;
    RmlCharacterCreatePanel(RmlCharacterCreatePanel&&) noexcept;
    RmlCharacterCreatePanel& operator=(RmlCharacterCreatePanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    void SetViewport(int width, int height) noexcept;
    [[nodiscard]] bool ApplyState(State state);
    [[nodiscard]] bool Update();

    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Character
