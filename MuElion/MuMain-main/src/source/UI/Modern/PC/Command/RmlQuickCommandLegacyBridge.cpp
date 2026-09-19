#include "stdafx.h"
#include "UI/Modern/PC/Command/RmlQuickCommandLegacyBridge.h"
#include "UI/Modern/PC/Command/RmlQuickCommandPanel.h"
#include "UI/NewUI/HUD/NewUIQuickCommandWindow.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace UI::Modern::PC::Command
{
namespace
{
std::string WideToUtf8(const std::wstring& input)
{
    std::string result;
    for(std::size_t i=0;i<input.size();++i)
    {
        char32_t cp=static_cast<char32_t>(input[i]);
        if constexpr(sizeof(wchar_t)==2)
        {
            const auto first=static_cast<std::uint16_t>(input[i]);
            if(first>=0xD800u&&first<=0xDBFFu&&i+1<input.size())
            {
                const auto second=static_cast<std::uint16_t>(input[i+1]);
                if(second>=0xDC00u&&second<=0xDFFFu)
                {
                    cp=0x10000u+((static_cast<char32_t>(first-0xD800u)<<10u)|
                        static_cast<char32_t>(second-0xDC00u));
                    ++i;
                }
            }
        }
        result+=Rml::StringUtilities::ToUTF8(static_cast<Rml::Character>(cp));
    }
    return result;
}
}

bool RmlQuickCommandLegacyBridge::Synchronize(
    const SEASON3B::CNewUIQuickCommandWindow& legacy,
    RmlQuickCommandPanel& modern)
{
    SEASON3B::QuickCommandSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);
    RmlQuickCommandPanel::State state;
    state.visible=snapshot.visible&&snapshot.selectedCharacterIndex>=0;
    state.viewportWidth=snapshot.viewportWidth;
    state.viewportHeight=snapshot.viewportHeight;
    state.x=static_cast<float>(snapshot.x);
    state.y=static_cast<float>(snapshot.y);
    state.targetName=WideToUtf8(snapshot.targetId);
    return modern.ApplyState(state);
}
} // namespace UI::Modern::PC::Command
