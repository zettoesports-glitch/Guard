#include "stdafx.h"
#include "UI/Modern/PC/Character/RmlCharacterCreateLegacyBridge.h"

#include "UI/Modern/PC/Character/RmlCharacterCreatePanel.h"
#include "Character/CharMakeWin.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace UI::Modern::PC::Character
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

void HashByte(std::uint64_t& h, std::uint8_t v) noexcept
{
    h ^= v;
    h *= 1099511628211ull;
}
void HashText(std::uint64_t& h, const std::wstring& v) noexcept
{
    for (wchar_t ch : v)
    {
        const auto raw = static_cast<std::uint32_t>(ch);
        for (unsigned shift = 0; shift < 32; shift += 8)
            HashByte(h, static_cast<std::uint8_t>(raw >> shift));
    }
    HashByte(h, 0xffu);
}
std::uint64_t Fingerprint(const CCharMakeSnapshot& s) noexcept
{
    std::uint64_t h = 1469598103934665603ull;
    HashByte(h, static_cast<std::uint8_t>(s.visible));
    HashByte(h, static_cast<std::uint8_t>(s.selectedClass));
    for (std::size_t i = 0; i < s.classEnabled.size(); ++i)
    {
        HashByte(h, static_cast<std::uint8_t>(s.classEnabled[i]));
        HashText(h, s.classNames[i]);
    }
    for (int i = 0; i < s.statCount; ++i)
    {
        HashText(h, s.statNames[static_cast<std::size_t>(i)]);
        HashText(h, s.statValues[static_cast<std::size_t>(i)]);
    }
    HashText(h, s.description);
    return h;
}
} // namespace

bool RmlCharacterCreateLegacyBridge::Synchronize(
    const CCharMakeWin& legacy,
    RmlCharacterCreatePanel& panel)
{
    CCharMakeSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);

    const std::uint64_t next = Fingerprint(snapshot);
    if (hasFingerprint_ && next == fingerprint_)
        return false;

    RmlCharacterCreatePanel::State state;
    state.visible = snapshot.visible;
    state.selectedClass = snapshot.selectedClass;
    state.classEnabled = snapshot.classEnabled;
    state.statCount = snapshot.statCount;
    state.description = ToUtf8(snapshot.description);

    for (std::size_t i = 0; i < state.classNames.size(); ++i)
        state.classNames[i] = ToUtf8(snapshot.classNames[i]);
    for (std::size_t i = 0; i < state.statNames.size(); ++i)
    {
        state.statNames[i] = ToUtf8(snapshot.statNames[i]);
        state.statValues[i] = ToUtf8(snapshot.statValues[i]);
    }

    const bool applied = panel.ApplyState(std::move(state));
    if (applied)
    {
        fingerprint_ = next;
        hasFingerprint_ = true;
    }
    return applied;
}

void RmlCharacterCreateLegacyBridge::Reset() noexcept
{
    fingerprint_ = 0;
    hasFingerprint_ = false;
}

} // namespace UI::Modern::PC::Character
