#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlMainFrameLegacyBridge.h"

#include "UI/Modern/PC/HUD/RmlMainFrameLayer.h"

#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <cstdint>

namespace UI::Modern::PC::HUD
{

namespace
{

using State = RmlMainFrameLayer::State;
using Gauge = RmlMainFrameLayer::Gauge;
using SkillIcon = RmlMainFrameLayer::SkillIcon;

Gauge MakeGauge(
    std::int64_t current,
    std::int64_t maximum) noexcept
{
    maximum = std::max<std::int64_t>(0, maximum);
    current = std::clamp<std::int64_t>(current, 0, maximum);
    return Gauge{current, maximum};
}

Gauge BuildExperienceGauge(bool masterExperience) noexcept
{
    if (!CharacterAttribute)
        return {};

    if (masterExperience)
    {
        const std::int64_t level =
            static_cast<std::int64_t>(Master_Level_Data.nMLevel);
        const std::int64_t totalLevel = level + 400;
        const std::int64_t overLevel = totalLevel - 255;

        const std::int64_t dataMaster =
            (9 + totalLevel) * totalLevel * totalLevel * 10 +
            (9 + overLevel) * overLevel * overLevel * 1000;
        const std::int64_t lowerBound =
            (dataMaster - 3892250000LL) / 2LL;
        const std::int64_t upperBound = std::max<std::int64_t>(
            lowerBound,
            static_cast<std::int64_t>(
                Master_Level_Data.lNext_MasterLevel_Experince));
        const std::int64_t experience = std::clamp<std::int64_t>(
            static_cast<std::int64_t>(
                Master_Level_Data.lMasterLevel_Experince),
            lowerBound,
            upperBound);

        return MakeGauge(
            experience - lowerBound,
            upperBound - lowerBound);
    }

    const std::int64_t level =
        static_cast<std::int64_t>(CharacterAttribute->Level);
    const std::int64_t priorLevel = level - 1;
    std::int64_t lowerBound = 0;

    if (priorLevel > 0)
    {
        lowerBound =
            (9 + priorLevel) * priorLevel * priorLevel * 10;
        if (priorLevel > 255)
        {
            const std::int64_t overLevel = priorLevel - 255;
            lowerBound +=
                (9 + overLevel) * overLevel * overLevel * 1000;
        }
    }

    const std::int64_t upperBound = std::max<std::int64_t>(
        lowerBound,
        static_cast<std::int64_t>(
            CharacterAttribute->NextExperience));
    const std::int64_t experience = std::clamp<std::int64_t>(
        static_cast<std::int64_t>(
            CharacterAttribute->Experience),
        lowerBound,
        upperBound);

    return MakeGauge(
        experience - lowerBound,
        upperBound - lowerBound);
}

float CooldownRatioForSkillIndex(int skillIndex) noexcept
{
    if (!CharacterAttribute || !SkillAttribute ||
        skillIndex < 0 || skillIndex >= MAX_MAGIC)
        return 0.0f;

    const int skillType =
        static_cast<int>(CharacterAttribute->Skill[skillIndex]);
    if (skillType <= 0)
        return 0.0f;

    const int maximumDelay = SkillAttribute[skillType].Delay;
    if (maximumDelay <= 0)
        return 0.0f;

    return std::clamp(
        static_cast<float>(
            CharacterAttribute->SkillDelay[skillIndex]) /
            static_cast<float>(maximumDelay),
        0.0f,
        1.0f);
}

SkillIcon BuildSkillIconFromIndex(int skillIndex) noexcept
{
    SkillIcon icon;
    if (!CharacterAttribute ||
        skillIndex < 0 || skillIndex >= MAX_MAGIC)
        return icon;

    const int skillType =
        static_cast<int>(CharacterAttribute->Skill[skillIndex]);
    if (skillType <= 0)
        return icon;

    icon.skillId = skillType;
    icon.cooldownRatio = CooldownRatioForSkillIndex(skillIndex);
    icon.visible = true;
    icon.enabled = true;
    return icon;
}

SkillIcon BuildSkillIconFromType(int skillType) noexcept
{
    if (!g_pSkillList || skillType <= 0)
        return {};

    const int skillIndex = g_pSkillList->GetSkillIndex(skillType);
    return BuildSkillIconFromIndex(skillIndex);
}

bool IsRenderableLegacySkill(int skillType) noexcept
{
    if (skillType <= 0 || !SkillAttribute)
        return false;

    if (skillType >= AT_SKILL_STUN &&
        skillType <= AT_SKILL_REMOVAL_BUFF)
        return false;

    const BYTE useType = SkillAttribute[skillType].SkillUseType;
    return useType != SKILL_USE_TYPE_MASTER &&
           useType != SKILL_USE_TYPE_MASTERLEVEL;
}

State BuildState() noexcept
{
    State state;
    state.viewportWidth = static_cast<int>(WindowWidth);
    state.viewportHeight = static_cast<int>(WindowHeight);

    if (!Hero || !CharacterAttribute ||
        !g_pMainFrame || !g_pSkillList)
        return state;

    state.visible = g_pMainFrame->IsVisible();

    const bool masterLevel =
        gCharacterManager.IsMasterLevel(Hero->Class);
    const bool masterExperience =
        gCharacterManager.IsMasterExperienceActive(
            CharacterAttribute->Class,
            CharacterAttribute->Level);

    if (masterLevel)
    {
        state.hp = MakeGauge(
            CharacterAttribute->Life,
            Master_Level_Data.wMaxLife);
        state.mp = MakeGauge(
            CharacterAttribute->Mana,
            Master_Level_Data.wMaxMana);
        state.sd = MakeGauge(
            CharacterAttribute->Shield,
            std::max<int>(1, Master_Level_Data.wMaxShield));
        state.ag = MakeGauge(
            CharacterAttribute->SkillMana,
            std::max<int>(1, Master_Level_Data.wMaxBP));
    }
    else
    {
        state.hp = MakeGauge(
            CharacterAttribute->Life,
            CharacterAttribute->LifeMax);
        state.mp = MakeGauge(
            CharacterAttribute->Mana,
            CharacterAttribute->ManaMax);
        state.sd = MakeGauge(
            CharacterAttribute->Shield,
            std::max<int>(1, CharacterAttribute->ShieldMax));
        state.ag = MakeGauge(
            CharacterAttribute->SkillMana,
            std::max<int>(1, CharacterAttribute->SkillManaMax));
    }

    state.experience = BuildExperienceGauge(masterExperience);
    state.experiencePage =
        state.experience.maximum > 0
            ? static_cast<int>(
                (state.experience.current * 10) /
                state.experience.maximum)
            : 0;
    state.experiencePage =
        std::clamp(state.experiencePage, 0, 9);
    state.experienceStyle =
        masterExperience
            ? RmlMainFrameLayer::ExperienceStyle::Master
            : RmlMainFrameLayer::ExperienceStyle::Normal;

    state.poisoned =
        g_isCharacterBuff(
            &Hero->Object,
            eDeBuff_Poison);

    // The legacy public method name is misleading: its body returns
    // m_bHotKeySkillListUp, i.e. the displayed 1..5 vs 6..0 page.
    state.secondSkillPage = g_pSkillList->IsSkillListUp();
    state.skillPageEnabled =
        CharacterAttribute->SkillNumber > 0;

    const int currentSkillIndex =
        static_cast<int>(Hero->CurrentSkill);
    state.skills[0] =
        BuildSkillIconFromIndex(currentSkillIndex);

    const int hotKeyStart =
        state.secondSkillPage ? 6 : 1;
    for (int visibleSlot = 0; visibleSlot < 5; ++visibleSlot)
    {
        int hotKey = hotKeyStart + visibleSlot;
        if (hotKey == 10)
            hotKey = 0;

        const int skillIndex =
            g_pMainFrame->GetSkillHotKey(hotKey);
        SkillIcon icon =
            BuildSkillIconFromIndex(skillIndex);

        // Match the legacy hotkey strip filter for master-level skills.
        if (icon.visible)
        {
            const int skillType = icon.skillId;
            if (SkillAttribute[skillType].SkillUseType ==
                SKILL_USE_TYPE_MASTERLEVEL)
                icon.visible = false;
        }

        state.skills[
            static_cast<std::size_t>(visibleSlot + 1)] = icon;

        if (skillIndex == currentSkillIndex && icon.visible)
            state.showCurrentSelection = true;
    }

    // The Debug/public API does not expose m_bSkillList, only the hotkey-page
    // latch above. Build the same filtered skill collection for a future
    // visibility bridge, but do not guess whether the chooser is currently open.
    state.skillListVisible = false;
    state.skillListSkills.clear();
    state.skillListSkills.reserve(
        static_cast<std::size_t>(CharacterAttribute->SkillNumber));

    for (int index = 0; index < MAX_MAGIC; ++index)
    {
        const int skillType =
            static_cast<int>(CharacterAttribute->Skill[index]);
        if (!IsRenderableLegacySkill(skillType))
            continue;

        SkillIcon icon = BuildSkillIconFromIndex(index);
        if (icon.visible)
            state.skillListSkills.push_back(icon);
    }

    // Hover/hot selection is private legacy event state and has no stable
    // public getter. Keep it hidden rather than infer from mouse coordinates.
    state.hotSelectionSlot = -1;

    return state;
}

} // namespace

bool RmlMainFrameLegacyBridge::Synchronize(
    RmlMainFrameLayer& panel) const
{
    if (!panel.IsLoaded())
        return false;

    return panel.ApplyState(BuildState());
}

} // namespace UI::Modern::PC::HUD
