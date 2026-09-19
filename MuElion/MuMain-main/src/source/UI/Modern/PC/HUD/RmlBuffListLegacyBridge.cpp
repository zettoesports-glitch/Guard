#include "stdafx.h"
#include "UI/Modern/PC/HUD/RmlBuffListLegacyBridge.h"

#include "UI/Modern/PC/HUD/RmlBuffListLayer.h"
#include "UI/NewUI/HUD/NewUIBuffWindow.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace UI::Modern::PC::HUD
{
namespace
{
std::string ToUtf8(const std::wstring& text)
{
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        char32_t cp = static_cast<char32_t>(text[i]);
        if constexpr (sizeof(wchar_t) == 2)
        {
            const auto first = static_cast<std::uint16_t>(text[i]);
            if (first >= 0xD800u && first <= 0xDBFFu &&
                i + 1u < text.size())
            {
                const auto second =
                    static_cast<std::uint16_t>(text[i + 1u]);
                if (second >= 0xDC00u && second <= 0xDFFFu)
                {
                    cp = 0x10000u +
                        ((static_cast<char32_t>(first - 0xD800u) << 10u) |
                         static_cast<char32_t>(second - 0xDC00u));
                    ++i;
                }
            }
        }
        result += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(cp));
    }
    return result;
}
void H(std::uint64_t& h, std::uint8_t v) noexcept
{
    h ^= v; h *= 1099511628211ull;
}
std::uint64_t Fingerprint(
    const SEASON3B::BuffWindowSnapshot& s) noexcept
{
    std::uint64_t h = 1469598103934665603ull;
    H(h, static_cast<std::uint8_t>(s.visible));
    for (const auto& e : s.entries)
    {
        const auto raw = static_cast<std::uint32_t>(e.buffState);
        H(h, static_cast<std::uint8_t>(raw));
        H(h, static_cast<std::uint8_t>(raw >> 8u));
        H(h, static_cast<std::uint8_t>(e.debuff));
        H(h, static_cast<std::uint8_t>(e.referenceCount));
        for (wchar_t ch : e.remainingTime)
        {
            const auto c = static_cast<std::uint32_t>(ch);
            H(h, static_cast<std::uint8_t>(c));
            H(h, static_cast<std::uint8_t>(c >> 8u));
        }
    }
    return h;
}
} // namespace

bool RmlBuffListLegacyBridge::Synchronize(
    SEASON3B::CNewUIBuffWindow& legacy,
    RmlBuffListLayer& layer)
{
    SEASON3B::BuffWindowSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);

    const std::uint64_t next = Fingerprint(snapshot);
    if (hasFingerprint_ && next == fingerprint_)
        return false;

    RmlBuffListLayer::State state;
    state.visible = snapshot.visible;
    state.entries.reserve(snapshot.entries.size());
    for (const auto& source : snapshot.entries)
    {
        RmlBuffListLayer::Entry entry;
        entry.buffState = source.buffState;
        entry.debuff = source.debuff;
        entry.referenceCount = source.referenceCount;
        entry.remainingTime = ToUtf8(source.remainingTime);
        state.entries.push_back(std::move(entry));
    }

    const bool applied = layer.ApplyState(std::move(state));
    if (applied)
    {
        fingerprint_ = next;
        hasFingerprint_ = true;
    }
    return applied;
}

void RmlBuffListLegacyBridge::Reset() noexcept
{
    fingerprint_ = 0;
    hasFingerprint_ = false;
}
} // namespace UI::Modern::PC::HUD
