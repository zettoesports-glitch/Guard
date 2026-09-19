#include "stdafx.h"
#include "UI/Modern/PC/MuHelper/RmlMuHelperLegacyBridge.h"

#include "UI/Modern/PC/MuHelper/RmlMuHelperPanel.h"

#include "Engine/Object/ZzzInfomation.h"
#include "MUHelper/MuHelper.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/NewUISystem.h"

#include <RmlUi/Core/StringUtilities.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UI::Modern::PC::MuHelper
{

namespace
{

std::string WideToUtf8(const std::wstring& input)
{
    std::string result;
    result.reserve(input.size());

    for (std::size_t index = 0; index < input.size(); ++index)
    {
        char32_t codePoint =
            static_cast<char32_t>(input[index]);

        if constexpr (sizeof(wchar_t) == 2)
        {
            const std::uint16_t first =
                static_cast<std::uint16_t>(input[index]);
            if (first >= 0xd800u && first <= 0xdbffu &&
                index + 1 < input.size())
            {
                const std::uint16_t second =
                    static_cast<std::uint16_t>(input[index + 1]);
                if (second >= 0xdc00u && second <= 0xdfffu)
                {
                    codePoint =
                        0x10000u +
                        ((static_cast<char32_t>(first - 0xd800u) << 10u) |
                         static_cast<char32_t>(second - 0xdc00u));
                    ++index;
                }
            }
        }

        result += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(codePoint));
    }

    return result;
}

RmlMuHelperPanel::State BuildState()
{
    RmlMuHelperPanel::State state;
    state.viewportWidth = static_cast<int>(WindowWidth);
    state.viewportHeight = static_cast<int>(WindowHeight);

    if (!g_pNewUISystem)
        return state;

    state.visible =
        g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MUHELPER);

    const MUHelper::ConfigData config =
        MUHelper::g_MuHelper.GetConfig();

    state.huntRange =
        std::clamp(config.iHuntingRange, 1, 8);
    state.obtainingRange =
        std::clamp(config.iObtainingRange, 1, 8);
    state.maxSecondsAway =
        std::max(0, config.iMaxSecondsAway);

    state.fallbackBasic = config.bFallbackBasicAttack;
    state.longCounter = config.bLongRangeCounterAttack;
    state.returnPosition = config.bReturnToOriginalPosition;
    state.usePotion = config.bUseHealPotion;
    state.supportParty = config.bSupportParty;
    state.autoHeal = config.bAutoHeal;
    state.drainLife = config.bUseDrainLife;
    state.buffDuration = config.bBuffDuration;
    state.useCombo = config.bUseCombo;
    state.useDarkRaven = config.bUseDarkRaven;

    state.repairItem = config.bRepairItem;
    state.pickAll = config.bPickAllItems;
    state.pickSelected = config.bPickSelectItems;
    state.pickJewel = config.bPickJewel;
    state.pickAncient = config.bPickAncient;
    state.pickZen = config.bPickZen;
    state.pickExcellent = config.bPickExcellent;
    state.pickExtra = config.bPickExtraItems;

    state.acceptFriend = config.bAutoAcceptFriend;
    state.acceptGuild = config.bAutoAcceptGuild;
    state.selfDefense = config.bUseSelfDefense;

    // The legacy helper exposes three attack skills and three buff skills.
    for (std::size_t index = 0; index < 3; ++index)
    {
        state.assignedSkills[index] =
            config.aiSkill[index] != 0u
                ? static_cast<int>(config.aiSkill[index])
                : -1;
        state.assignedSkills[index + 3] =
            config.aiBuff[index] != 0u
                ? static_cast<int>(config.aiBuff[index])
                : -1;
    }

    state.extraItems.clear();
    state.extraItems.reserve(config.aExtraItems.size());
    for (const std::wstring& item : config.aExtraItems)
        state.extraItems.push_back(WideToUtf8(item));

    // The available-skill chooser is presentation-only and the legacy helper
    // filters it differently for attack/buff subwindows. Without knowing which
    // subwindow is currently active, do not manufacture one mixed list.
    state.availableSkills.clear();

    // The three legacy main tabs do not have a public current-tab getter.
    // Keep the panel's retained tab unless/until that private state is
    // recovered; State defaults to Hunt for first load.
    return state;
}

} // namespace

bool RmlMuHelperLegacyBridge::Synchronize(
    RmlMuHelperPanel& panel) const
{
    if (!panel.IsLoaded())
        return false;

    return panel.ApplyState(BuildState());
}

} // namespace UI::Modern::PC::MuHelper
