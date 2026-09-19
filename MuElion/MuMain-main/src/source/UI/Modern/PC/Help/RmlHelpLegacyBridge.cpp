#include "stdafx.h"
#include "UI/Modern/PC/Help/RmlHelpLegacyBridge.h"

#include "UI/Modern/PC/Help/RmlHelpPanel.h"
#include "UI/NewUI/Dialogs/NewUIHelpWindow.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace UI::Modern::PC::Help
{
namespace
{
std::string WideToUtf8(const std::wstring& input)
{
    std::string result;
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        char32_t codePoint = static_cast<char32_t>(input[i]);
        if constexpr (sizeof(wchar_t) == 2)
        {
            const auto first = static_cast<std::uint16_t>(input[i]);
            if (first >= 0xD800u && first <= 0xDBFFu &&
                i + 1u < input.size())
            {
                const auto second =
                    static_cast<std::uint16_t>(input[i + 1u]);
                if (second >= 0xDC00u && second <= 0xDFFFu)
                {
                    codePoint =
                        0x10000u +
                        ((static_cast<char32_t>(first - 0xD800u) << 10u) |
                         static_cast<char32_t>(second - 0xDC00u));
                    ++i;
                }
            }
        }
        result += Rml::StringUtilities::ToUTF8(
            static_cast<Rml::Character>(codePoint));
    }
    return result;
}

void HashByte(std::uint64_t& hash, std::uint8_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ull;
}

void HashWide(std::uint64_t& hash, const std::wstring& value) noexcept
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

std::uint64_t Fingerprint(
    const SEASON3B::HelpSnapshot& snapshot,
    int viewportWidth,
    int viewportHeight) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    HashByte(hash, static_cast<std::uint8_t>(snapshot.visible));
    HashByte(hash, static_cast<std::uint8_t>(snapshot.selectedPage));
    for (int shift = 0; shift < 32; shift += 8)
    {
        HashByte(hash, static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(snapshot.x) >> shift));
        HashByte(hash, static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(snapshot.y) >> shift));
        HashByte(hash, static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(viewportWidth) >> shift));
        HashByte(hash, static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(viewportHeight) >> shift));
    }
    for (const auto& page : snapshot.pages)
    {
        HashWide(hash, page.heading);
        for (const auto& row : page.rows)
            HashWide(hash, row);
    }
    return hash;
}
} // namespace

bool RmlHelpLegacyBridge::Synchronize(
    const SEASON3B::CNewUIHelpWindow& legacy,
    RmlHelpPanel& panel,
    int viewportWidth,
    int viewportHeight)
{
    SEASON3B::HelpSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);

    const std::uint64_t fingerprint =
        Fingerprint(snapshot, viewportWidth, viewportHeight);
    if (hasFingerprint_ && fingerprint == lastFingerprint_)
        return false;

    RmlHelpPanel::State state;
    state.visible = snapshot.visible;
    state.viewportWidth = viewportWidth;
    state.viewportHeight = viewportHeight;
    state.x = static_cast<float>(snapshot.x);
    state.y = static_cast<float>(snapshot.y);
    state.selectedPage = snapshot.selectedPage;

    for (std::size_t page = 0; page < snapshot.pages.size(); ++page)
    {
        state.pages[page].heading =
            WideToUtf8(snapshot.pages[page].heading);
        state.pages[page].rows.reserve(
            snapshot.pages[page].rows.size());
        for (const auto& row : snapshot.pages[page].rows)
            state.pages[page].rows.push_back(WideToUtf8(row));
    }

    const bool changed = panel.ApplyState(state);
    lastFingerprint_ = fingerprint;
    hasFingerprint_ = true;
    return changed;
}

void RmlHelpLegacyBridge::Reset() noexcept
{
    lastFingerprint_ = 0;
    hasFingerprint_ = false;
}

} // namespace UI::Modern::PC::Help
