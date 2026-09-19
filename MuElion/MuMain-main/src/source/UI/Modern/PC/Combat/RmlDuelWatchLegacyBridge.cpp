#include "stdafx.h"
#include "UI/Modern/PC/Combat/RmlDuelWatchLegacyBridge.h"
#include "UI/Modern/PC/Combat/RmlDuelWatchPanel.h"
#include "UI/NewUI/Combat/NewUIDuelWatchWindow.h"

#include <RmlUi/Core/StringUtilities.h>
#include <cstddef>
#include <cstdint>
#include <string>

namespace UI::Modern::PC::Combat
{
namespace
{
std::string WideToUtf8(const std::wstring& input)
{
    std::string out;
    for(std::size_t i=0;i<input.size();++i)
    {
        char32_t cp=static_cast<char32_t>(input[i]);
        if constexpr(sizeof(wchar_t)==2)
        {
            const auto a=static_cast<std::uint16_t>(input[i]);
            if(a>=0xD800u&&a<=0xDBFFu&&i+1<input.size())
            {
                const auto b=static_cast<std::uint16_t>(input[i+1]);
                if(b>=0xDC00u&&b<=0xDFFFu)
                {cp=0x10000u+((static_cast<char32_t>(a-0xD800u)<<10u)|static_cast<char32_t>(b-0xDC00u));++i;}
            }
        }
        out+=Rml::StringUtilities::ToUTF8(static_cast<Rml::Character>(cp));
    }
    return out;
}
}

bool RmlDuelWatchLegacyBridge::Synchronize(
    const SEASON3B::CNewUIDuelWatchWindow& legacy,
    RmlDuelWatchPanel& modern)
{
    SEASON3B::DuelWatchSnapshot snapshot; legacy.BuildSnapshot(snapshot);
    RmlDuelWatchPanel::State state;
    state.visible=snapshot.visible;state.viewportWidth=snapshot.viewportWidth;state.viewportHeight=snapshot.viewportHeight;
    state.x=static_cast<float>(snapshot.x);state.y=static_cast<float>(snapshot.y);
    for(std::size_t i=0;i<state.channels.size();++i)
    {
        state.channels[i].enabled=snapshot.channels[i].enabled;
        state.channels[i].joinable=snapshot.channels[i].joinable;
        state.channels[i].user1=WideToUtf8(snapshot.channels[i].user1);
        state.channels[i].user2=WideToUtf8(snapshot.channels[i].user2);
    }
    return modern.ApplyState(state);
}
}
