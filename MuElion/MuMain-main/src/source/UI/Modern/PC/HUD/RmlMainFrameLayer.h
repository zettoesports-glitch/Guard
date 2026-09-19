#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace UI::Modern::PC::HUD
{

class RmlMainFrameLayer
{
public:
    enum class ExperienceStyle : std::uint8_t
    {
        Normal = 0,
        Master = 1,
        Fourth = 2,
    };

    enum class Action : std::uint8_t
    {
        Shop,
        Character,
        Inventory,
        Quest,
        Community,
        System,
        ToggleSkillPage,
    };

    struct Gauge
    {
        std::int64_t current = 0;
        std::int64_t maximum = 0;
    };

    struct SkillIcon
    {
        int skillId = -1;
        float cooldownRatio = 0.0f;
        bool visible = false;
        bool enabled = true;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        Gauge hp;
        Gauge mp;
        Gauge sd;
        Gauge ag;
        Gauge experience;

        int experiencePage = 0;
        ExperienceStyle experienceStyle = ExperienceStyle::Normal;
        bool poisoned = false;

        bool secondSkillPage = false;
        bool skillPageEnabled = true;
        bool showCurrentSelection = false;
        int hotSelectionSlot = -1;

        std::array<SkillIcon, 6> skills{};

        // Filtered non-master skills shown by the legacy skill chooser.
        bool skillListVisible = false;
        std::vector<SkillIcon> skillListSkills;
    };

    RmlMainFrameLayer();
    ~RmlMainFrameLayer();

    RmlMainFrameLayer(const RmlMainFrameLayer&) = delete;
    RmlMainFrameLayer& operator=(const RmlMainFrameLayer&) = delete;
    RmlMainFrameLayer(RmlMainFrameLayer&&) noexcept;
    RmlMainFrameLayer& operator=(RmlMainFrameLayer&&) noexcept;

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

} // namespace UI::Modern::PC::HUD
