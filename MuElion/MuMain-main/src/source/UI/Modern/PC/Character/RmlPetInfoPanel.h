#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Character
{

class RmlPetInfoPanel
{
public:
    enum class ActionType
    {
        SelectTab,
        Close,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t tabIndex = 0;
    };

    struct Field
    {
        std::string label;
        std::string value;
        bool visible = true;
    };

    struct Skill
    {
        int skillId = 0;
        std::string label;
        bool enabled = true;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string title = "Pet";
        std::array<std::string, 2> tabLabels{
            "Dark Horse", "Dark Raven"};
        std::size_t selectedTab = 0;

        bool missing = false;
        std::string missingText;

        std::array<Field, 5> fields{};
        float progressRatio = 0.0f;

        bool commandsVisible = false;
        std::string leadershipLabel;
        std::string leadershipValue;
        std::string commandLabel;
        std::vector<Skill> skills;
    };

    RmlPetInfoPanel();
    ~RmlPetInfoPanel();

    RmlPetInfoPanel(const RmlPetInfoPanel&) = delete;
    RmlPetInfoPanel& operator=(const RmlPetInfoPanel&) = delete;
    RmlPetInfoPanel(RmlPetInfoPanel&&) noexcept;
    RmlPetInfoPanel& operator=(RmlPetInfoPanel&&) noexcept;

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

} // namespace UI::Modern::PC::Character
