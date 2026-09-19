#pragma once

#include "MUHelper/MuHelperData.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::MuHelper
{

class RmlMuHelperPanel
{
public:
    enum class Tab
    {
        Hunt,
        Item,
        Other,
    };

    enum class Action
    {
        Save,
        Reset,
        Close,
        PotionSettings,
        Skill1Settings,
        Skill2Settings,
        PartySettings,
    };

    struct SkillSlotRequest
    {
        bool available = false;
        std::size_t index = 0;
    };

    RmlMuHelperPanel();
    ~RmlMuHelperPanel();

    RmlMuHelperPanel(const RmlMuHelperPanel&) = delete;
    RmlMuHelperPanel& operator=(const RmlMuHelperPanel&) = delete;
    RmlMuHelperPanel(RmlMuHelperPanel&&) noexcept;
    RmlMuHelperPanel& operator=(RmlMuHelperPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();
    [[nodiscard]] bool IsLoaded() const noexcept;

    void SetConfig(const MUHelper::ConfigData& config);
    [[nodiscard]] MUHelper::ConfigData GetConfig() const;

    void SetTab(Tab tab);
    [[nodiscard]] Tab GetTab() const noexcept;

    void SetAssignedSkillFrame(std::size_t index, int frame);
    void SetAvailableSkillFrame(std::size_t index, int frame);

    [[nodiscard]] bool Update();

    [[nodiscard]] std::optional<Action> ConsumeAction();
    [[nodiscard]] std::optional<SkillSlotRequest> ConsumeSkillSlotRequest();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::MuHelper
