#include "stdafx.h"
#include "UI/Modern/PC/Chat/RmlChatLegacyBridge.h"

#include "UI/Modern/PC/Chat/RmlChatPanel.h"
#include "UI/NewUI/HUD/NewUIChatLogWindow.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace UI::Modern::PC::Chat
{

namespace
{
std::string WideToUtf8(const std::wstring& input)
{
    std::string result;
    result.reserve(input.size());

    for (std::size_t index = 0; index < input.size(); ++index)
    {
        char32_t codePoint = static_cast<char32_t>(input[index]);

        if constexpr (sizeof(wchar_t) == 2)
        {
            const std::uint16_t first =
                static_cast<std::uint16_t>(input[index]);
            if (first >= 0xD800u && first <= 0xDBFFu &&
                index + 1u < input.size())
            {
                const std::uint16_t second =
                    static_cast<std::uint16_t>(input[index + 1u]);
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

RmlChatPanel::MessageKind MapKind(SEASON3B::MESSAGE_TYPE type) noexcept
{
    using K = RmlChatPanel::MessageKind;
    switch (type)
    {
    case SEASON3B::TYPE_WHISPER_MESSAGE: return K::Whisper;
    case SEASON3B::TYPE_SYSTEM_MESSAGE: return K::System;
    case SEASON3B::TYPE_ERROR_MESSAGE: return K::Error;
    case SEASON3B::TYPE_PARTY_MESSAGE: return K::Party;
    case SEASON3B::TYPE_GUILD_MESSAGE: return K::Guild;
    case SEASON3B::TYPE_UNION_MESSAGE: return K::Union;
    case SEASON3B::TYPE_GM_MESSAGE: return K::Gm;
    case SEASON3B::TYPE_GENS_MESSAGE: return K::Gens;
    case SEASON3B::TYPE_CHAT_MESSAGE:
    case SEASON3B::TYPE_ALL_MESSAGE:
    case SEASON3B::TYPE_UNKNOWN:
    default:
        return K::Normal;
    }
}

void HashByte(std::uint64_t& hash, std::uint8_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ull;
}

void HashString(std::uint64_t& hash, const std::wstring& value) noexcept
{
    for (wchar_t ch : value)
    {
        const auto raw = static_cast<std::uint32_t>(ch);
        HashByte(hash, static_cast<std::uint8_t>(raw));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 8u));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 16u));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 24u));
    }
    HashByte(hash, 0xffu);
}

std::uint64_t Fingerprint(const SEASON3B::ChatLogSnapshot& snapshot) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    for (const auto& line : snapshot.lines)
    {
        HashString(hash, line.id);
        HashString(hash, line.text);
        HashByte(hash, static_cast<std::uint8_t>(line.type));
    }

    HashByte(hash, static_cast<std::uint8_t>(snapshot.showChatLog));
    HashByte(hash, static_cast<std::uint8_t>(snapshot.showFrame));
    return hash;
}
} // namespace

bool RmlChatLegacyBridge::Synchronize(
    const SEASON3B::CNewUIChatLogWindow& legacy,
    RmlChatPanel& panel)
{
    SEASON3B::ChatLogSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);

    const std::uint64_t fingerprint = Fingerprint(snapshot);
    if (hasFingerprint_ && fingerprint == lastFingerprint_)
    {
        panel.SetVisible(snapshot.showChatLog);
        return false;
    }

    std::vector<RmlChatPanel::Message> messages;
    messages.reserve(snapshot.lines.size());
    for (const auto& line : snapshot.lines)
    {
        RmlChatPanel::Message message;
        message.sender = WideToUtf8(line.id);
        message.text = WideToUtf8(line.text);
        message.kind = MapKind(line.type);
        messages.push_back(std::move(message));
    }

    panel.SetMessages(std::move(messages));
    panel.SetVisible(snapshot.showChatLog);

    lastFingerprint_ = fingerprint;
    hasFingerprint_ = true;
    return true;
}

void RmlChatLegacyBridge::Reset() noexcept
{
    lastFingerprint_ = 0;
    hasFingerprint_ = false;
}

} // namespace UI::Modern::PC::Chat
