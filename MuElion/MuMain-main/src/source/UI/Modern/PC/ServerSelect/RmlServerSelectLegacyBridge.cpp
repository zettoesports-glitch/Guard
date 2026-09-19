#include "stdafx.h"
#include "UI/Modern/PC/ServerSelect/RmlServerSelectLegacyBridge.h"

#include "UI/Modern/PC/ServerSelect/RmlServerSelectPanel.h"
#include "Network/Server/ServerListManager.h"

#include <RmlUi/Core/StringUtilities.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace UI::Modern::PC::ServerSelect
{

namespace
{
std::string WideToUtf8(const wchar_t* text)
{
    if (!text)
        return {};

    std::string result;
    for (std::size_t index = 0; text[index] != 0; ++index)
    {
        char32_t codePoint = static_cast<char32_t>(text[index]);
        if constexpr (sizeof(wchar_t) == 2)
        {
            const auto first = static_cast<std::uint16_t>(text[index]);
            if (first >= 0xD800u && first <= 0xDBFFu &&
                text[index + 1] != 0)
            {
                const auto second =
                    static_cast<std::uint16_t>(text[index + 1]);
                if (second >= 0xDC00u && second <= 0xDFFFu)
                {
                    codePoint = 0x10000u +
                        ((static_cast<char32_t>(first - 0xD800u) << 10u) |
                         static_cast<char32_t>(second - 0xDC00u));
                    ++index;
                }
            }
        }

        result += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(codePoint));
    }
    return result;
}

RmlServerSelectPanel::ServerKind KindFromByte(BYTE value) noexcept
{
    using K = RmlServerSelectPanel::ServerKind;
    switch (value)
    {
    case 1: return K::NonPvp;
    case 2: return K::GoldPvp;
    case 3: return K::Gold;
    case 0:
    default: return K::Pvp;
    }
}

void HashByte(std::uint64_t& hash, std::uint8_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ull;
}

void HashInt(std::uint64_t& hash, int value) noexcept
{
    const auto raw = static_cast<std::uint32_t>(value);
    for (unsigned shift = 0; shift < 32; shift += 8)
        HashByte(hash, static_cast<std::uint8_t>(raw >> shift));
}

void HashText(std::uint64_t& hash, const std::string& value) noexcept
{
    for (unsigned char ch : value)
        HashByte(hash, ch);
    HashByte(hash, 0xffu);
}

std::uint64_t Fingerprint(
    const RmlServerSelectPanel::State& state) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    HashInt(hash, state.selectedGroupKey);
    for (const auto& group : state.groups)
    {
        HashInt(hash, group.key);
        HashText(hash, group.name);
        HashText(hash, group.description);
        for (const auto& server : group.servers)
        {
            HashInt(hash, server.connectIndex);
            HashInt(hash, server.serverIndex);
            HashInt(hash, server.loadPercent);
            HashByte(hash, static_cast<std::uint8_t>(server.kind));
            HashByte(hash, static_cast<std::uint8_t>(server.enabled));
            HashText(hash, server.name);
        }
    }
    return hash;
}
} // namespace

bool RmlServerSelectLegacyBridge::Synchronize(
    RmlServerSelectPanel& panel)
{
    RmlServerSelectPanel::State state;
    state.title = "Server Selection";

    const CServerListManager* manager = g_ServerListManager;
    if (!manager)
        return false;

    state.groups.reserve(manager->m_mapServerGroup.size());

    for (const auto& [sequence, groupPtr] : manager->m_mapServerGroup)
    {
        const CServerGroup* group = groupPtr;
        if (!group)
            continue;

        RmlServerSelectPanel::Group modernGroup;
        modernGroup.key = sequence;
        modernGroup.name = WideToUtf8(group->m_szName);
        modernGroup.description =
            WideToUtf8(group->m_szDescription);
        modernGroup.servers.reserve(group->m_listServerInfo.size());

        for (const CServerInfo* server : group->m_listServerInfo)
        {
            if (!server)
                continue;

            RmlServerSelectPanel::Server modernServer;
            modernServer.name = WideToUtf8(server->m_bName);
            modernServer.connectIndex = server->m_iConnectIndex;
            modernServer.serverIndex = server->m_iIndex;
            modernServer.loadPercent =
                std::clamp(server->m_iPercent, 0, 128);
            modernServer.kind = KindFromByte(server->m_byNonPvP);
            modernServer.enabled = server->m_iPercent < 128;
            modernGroup.servers.push_back(std::move(modernServer));
        }

        state.groups.push_back(std::move(modernGroup));
    }

    if (!state.groups.empty())
        state.selectedGroupKey = state.groups.front().key;

    const std::uint64_t next = Fingerprint(state);
    if (hasFingerprint_ && next == fingerprint_)
        return false;

    const bool applied = panel.ApplyState(std::move(state));
    if (applied)
    {
        fingerprint_ = next;
        hasFingerprint_ = true;
    }
    return applied;
}

void RmlServerSelectLegacyBridge::Reset() noexcept
{
    fingerprint_ = 0;
    hasFingerprint_ = false;
}

} // namespace UI::Modern::PC::ServerSelect
