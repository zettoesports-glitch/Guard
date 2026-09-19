#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::HUD
{

class RmlMasterTreePanel
{
public:
    static constexpr std::size_t SlotCount = 108;
    static constexpr std::size_t CategoryCount = 3;

    enum class ActionType
    {
        UpgradeSkill,
        Experience,
        Close,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        int skillId = -1;
    };

    struct SkillSlot
    {
        // Authored modern slot id, in the recovered range 1..108.
        std::size_t slotId = 0;

        int skillId = -1;
        int group = 0;
        int rank = 1;
        int column = 0;
        int direction = 0;

        int level = 0;
        int maxLevel = 0;

        bool visible = false;
        bool enabled = false;

        // Optional generated RCSS class supplied by the game-side skill/icon
        // mapper. Empty leaves the generic reconstructed icon.
        std::string iconClass;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        bool applyPosition = false;
        float positionX = 0.0f;
        float positionY = 0.0f;

        std::string experience;
        std::string points;
        std::string level;
        std::string characterClass;
        std::array<std::string, CategoryCount> categoryLabels{};

        std::vector<SkillSlot> skills;
    };

    RmlMasterTreePanel();
    ~RmlMasterTreePanel();

    RmlMasterTreePanel(const RmlMasterTreePanel&) = delete;
    RmlMasterTreePanel& operator=(const RmlMasterTreePanel&) = delete;
    RmlMasterTreePanel(RmlMasterTreePanel&&) noexcept;
    RmlMasterTreePanel& operator=(RmlMasterTreePanel&&) noexcept;

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
