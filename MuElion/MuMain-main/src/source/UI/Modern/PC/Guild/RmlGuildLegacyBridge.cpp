#include "stdafx.h"
#include "UI/Modern/PC/Guild/RmlGuildLegacyBridge.h"

#include "Guild/NewUIGuildInfoWindow.h"
#include "Guild/NewUIGuildMakeWindow.h"
#include "Guild/UIGuildInfo.h"
#include "UI/Modern/PC/Guild/RmlGuildCreatePanel.h"
#include "UI/Modern/PC/Guild/RmlGuildInfoPanel.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstdint>
#include <string>
#include <utility>

namespace UI::Modern::PC::Guild
{
namespace
{
std::string Utf8(const std::wstring& input)
{
    std::string output;
    output.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        char32_t cp = static_cast<char32_t>(input[i]);
        if constexpr (sizeof(wchar_t) == 2)
        {
            const auto first = static_cast<std::uint16_t>(input[i]);
            if (first >= 0xd800u && first <= 0xdbffu &&
                i + 1u < input.size())
            {
                const auto second =
                    static_cast<std::uint16_t>(input[i + 1u]);
                if (second >= 0xdc00u && second <= 0xdfffu)
                {
                    cp = 0x10000u +
                        ((static_cast<char32_t>(first - 0xd800u) << 10u) |
                         static_cast<char32_t>(second - 0xdc00u));
                    ++i;
                }
            }
        }
        output += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(cp));
    }
    return output;
}

void HashByte(std::uint64_t& hash, std::uint8_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ull;
}
template <typename T>
void HashPod(std::uint64_t& hash, T value) noexcept
{
    const auto* bytes =
        reinterpret_cast<const std::uint8_t*>(&value);
    for (std::size_t i = 0; i < sizeof(T); ++i)
        HashByte(hash, bytes[i]);
}
void HashWide(std::uint64_t& hash, const std::wstring& value) noexcept
{
    for (wchar_t ch : value)
        HashPod(hash, static_cast<std::uint32_t>(ch));
    HashByte(hash, 0xffu);
}

std::uint64_t Fingerprint(
    const SEASON3B::GuildCreateSnapshot& snapshot) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    HashPod(hash, snapshot.visible);
    HashPod(hash, snapshot.x);
    HashPod(hash, snapshot.y);
    HashPod(hash, snapshot.page);
    HashWide(hash, snapshot.guildName);
    HashByte(hash, snapshot.selectedColor);
    for (const auto color : snapshot.mark)
        HashByte(hash, color);
    return hash;
}

std::uint64_t Fingerprint(
    const SEASON3B::GuildInfoSnapshot& snapshot) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    HashPod(hash, snapshot.visible);
    HashPod(hash, snapshot.x);
    HashPod(hash, snapshot.y);
    HashPod(hash, snapshot.tab);
    HashPod(hash, snapshot.hasGuild);
    HashPod(hash, snapshot.isGuildMaster);
    HashWide(hash, snapshot.guildName);
    HashPod(hash, snapshot.guildScore);
    HashPod(hash, snapshot.memberCount);
    HashPod(hash, snapshot.memberCapacity);
    HashWide(hash, snapshot.rivalGuild);
    for (const auto color : snapshot.guildMark)
        HashByte(hash, color);
    for (const auto& notice : snapshot.notices)
        HashWide(hash, notice);
    for (const auto& member : snapshot.members)
    {
        HashWide(hash, member.name);
        HashByte(hash, member.number);
        HashByte(hash, member.server);
        HashByte(hash, member.status);
    }
    for (const auto& guild : snapshot.unions)
    {
        HashWide(hash, guild.name);
        HashPod(hash, guild.memberCount);
        for (const auto color : guild.mark)
            HashByte(hash, color);
    }
    return hash;
}

std::string StatusName(std::uint8_t status)
{
    switch (status)
    {
    case G_MASTER: return "Master";
    case G_SUB_MASTER: return "Assistant";
    case G_BATTLE_MASTER: return "Battle Master";
    default: return "Member";
    }
}

RmlGuildInfoPanel::Tab MapTab(int tab) noexcept
{
    if (tab == static_cast<int>(GuildConstants::GuildTab::INFO))
        return RmlGuildInfoPanel::Tab::Info;
    if (tab == static_cast<int>(GuildConstants::GuildTab::UNION))
        return RmlGuildInfoPanel::Tab::Union;
    return RmlGuildInfoPanel::Tab::Members;
}
} // namespace

bool RmlGuildLegacyBridge::SynchronizeCreate(
    SEASON3B::CNewUIGuildMakeWindow& legacy,
    RmlGuildCreatePanel& panel,
    int viewportWidth,
    int viewportHeight)
{
    SEASON3B::GuildCreateSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);
    const auto fingerprint = Fingerprint(snapshot);
    if (hasCreateFingerprint_ &&
        fingerprint == createFingerprint_)
        return false;

    RmlGuildCreatePanel::State state;
    state.visible = snapshot.visible;
    state.viewportWidth = viewportWidth;
    state.viewportHeight = viewportHeight;
    state.x = static_cast<float>(snapshot.x);
    state.y = static_cast<float>(snapshot.y);

    if (snapshot.page ==
        SEASON3B::CNewUIGuildMakeWindow::GUILDMAKE_MARK)
        state.page = RmlGuildCreatePanel::Page::Mark;
    else if (snapshot.page ==
             SEASON3B::CNewUIGuildMakeWindow::GUILDMAKE_RESULTINFO)
        state.page = RmlGuildCreatePanel::Page::Confirm;
    else
        state.page = RmlGuildCreatePanel::Page::Intro;

    state.guildName = Utf8(snapshot.guildName);
    state.mark = snapshot.mark;
    state.selectedColor = snapshot.selectedColor;

    const bool changed = panel.ApplyState(state);
    createFingerprint_ = fingerprint;
    hasCreateFingerprint_ = true;
    return changed;
}

bool RmlGuildLegacyBridge::SynchronizeInfo(
    const SEASON3B::CNewUIGuildInfoWindow& legacy,
    RmlGuildInfoPanel& panel,
    int viewportWidth,
    int viewportHeight)
{
    SEASON3B::GuildInfoSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);
    const auto fingerprint = Fingerprint(snapshot);
    if (hasInfoFingerprint_ &&
        fingerprint == infoFingerprint_)
        return false;

    RmlGuildInfoPanel::State state;
    state.visible = snapshot.visible;
    state.viewportWidth = viewportWidth;
    state.viewportHeight = viewportHeight;
    state.x = static_cast<float>(snapshot.x);
    state.y = static_cast<float>(snapshot.y);
    state.hasGuild = snapshot.hasGuild;
    state.isGuildMaster = snapshot.isGuildMaster;
    state.tab = MapTab(snapshot.tab);
    state.guildName = Utf8(snapshot.guildName);
    state.guildScore = snapshot.guildScore;
    state.memberCount = snapshot.memberCount;
    state.memberCapacity = snapshot.memberCapacity;
    state.rivalGuild = Utf8(snapshot.rivalGuild);
    state.guildMark = snapshot.guildMark;

    for (const auto& line : snapshot.notices)
    {
        if (!state.notice.empty())
            state.notice.push_back('\n');
        state.notice += Utf8(line);
    }

    state.members.reserve(snapshot.members.size());
    for (const auto& member : snapshot.members)
    {
        RmlGuildInfoPanel::Member output;
        output.name = Utf8(member.name);
        output.position = StatusName(member.status);
        output.server =
            std::to_string(static_cast<unsigned>(member.server));
        state.members.push_back(std::move(output));
    }

    state.unions.reserve(snapshot.unions.size());
    for (const auto& guild : snapshot.unions)
    {
        RmlGuildInfoPanel::UnionGuild output;
        output.name = Utf8(guild.name);
        output.memberCount = guild.memberCount;
        output.mark = guild.mark;
        state.unions.push_back(std::move(output));
    }

    if (!state.rivalGuild.empty())
        state.rivals.push_back(state.rivalGuild);

    const bool changed = panel.ApplyState(state);
    infoFingerprint_ = fingerprint;
    hasInfoFingerprint_ = true;
    return changed;
}

void RmlGuildLegacyBridge::Reset() noexcept
{
    createFingerprint_ = 0;
    infoFingerprint_ = 0;
    hasCreateFingerprint_ = false;
    hasInfoFingerprint_ = false;
}

} // namespace UI::Modern::PC::Guild
