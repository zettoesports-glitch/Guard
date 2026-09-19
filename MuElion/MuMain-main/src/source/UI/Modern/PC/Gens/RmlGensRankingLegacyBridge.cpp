#include "stdafx.h"
#include "UI/Modern/PC/Gens/RmlGensRankingLegacyBridge.h"

#include "UI/Modern/PC/Gens/RmlGensRankingPanel.h"
#include "UI/NewUI/HUD/NewUIGensRanking.h"

#include <RmlUi/Core/StringUtilities.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace UI::Modern::PC::Gens
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
        const std::uint32_t raw = static_cast<std::uint32_t>(ch);
        HashByte(hash, static_cast<std::uint8_t>(raw));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 8u));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 16u));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 24u));
    }
    HashByte(hash, 0xffu);
}

#ifdef PBG_ADD_GENSRANKING
std::uint64_t Fingerprint(
    const SEASON3B::GensRankingSnapshot& snapshot,
    int viewportWidth,
    int viewportHeight) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    const auto hashInt = [&](std::int32_t value) {
        const auto raw = static_cast<std::uint32_t>(value);
        for (int shift = 0; shift < 32; shift += 8)
            HashByte(hash, static_cast<std::uint8_t>(raw >> shift));
    };

    HashByte(hash, static_cast<std::uint8_t>(snapshot.visible));
    hashInt(snapshot.x);
    hashInt(snapshot.y);
    hashInt(snapshot.influence);
    hashInt(snapshot.rankIndex);
    hashInt(snapshot.contribution);
    hashInt(snapshot.nextContribution);
    hashInt(viewportWidth);
    hashInt(viewportHeight);

    HashWide(hash, snapshot.title);
    HashWide(hash, snapshot.gensInfoLabel);
    HashWide(hash, snapshot.gensTeamLabel);
    HashWide(hash, snapshot.gensRankingLabel);
    HashWide(hash, snapshot.rankingLabel);
    HashWide(hash, snapshot.contributionLabel);
    HashWide(hash, snapshot.teamValue);
    HashWide(hash, snapshot.gradeValue);
    HashWide(hash, snapshot.rankingValue);
    HashWide(hash, snapshot.contributionValue);
    HashWide(hash, snapshot.promotionText);
    for (const auto& line : snapshot.infoLines)
        HashWide(hash, line);

    return hash;
}
#endif // PBG_ADD_GENSRANKING
} // namespace

bool RmlGensRankingLegacyBridge::Synchronize(
    const SEASON3B::CNewUIGensRanking& legacy,
    RmlGensRankingPanel& panel,
    int viewportWidth,
    int viewportHeight)
{
#ifdef PBG_ADD_GENSRANKING
    SEASON3B::GensRankingSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);

    const std::uint64_t fingerprint =
        Fingerprint(snapshot, viewportWidth, viewportHeight);
    if (hasFingerprint_ && fingerprint == lastFingerprint_)
        return false;

    RmlGensRankingPanel::State state;
    state.visible = snapshot.visible;
    state.viewportWidth = viewportWidth;
    state.viewportHeight = viewportHeight;
    state.x = static_cast<float>(snapshot.x);
    state.y = static_cast<float>(snapshot.y);
    state.influence =
        snapshot.influence == 1
            ? RmlGensRankingPanel::Influence::Duprian
            : snapshot.influence == 2
                ? RmlGensRankingPanel::Influence::Vanert
                : RmlGensRankingPanel::Influence::None;
    state.rankIndex = snapshot.rankIndex;

    state.title = WideToUtf8(snapshot.title);
    state.gensInfoLabel = WideToUtf8(snapshot.gensInfoLabel);
    state.gensTeamLabel = WideToUtf8(snapshot.gensTeamLabel);
    state.gensRankingLabel = WideToUtf8(snapshot.gensRankingLabel);
    state.rankingLabel = WideToUtf8(snapshot.rankingLabel);
    state.contributionLabel = WideToUtf8(snapshot.contributionLabel);
    state.teamValue = WideToUtf8(snapshot.teamValue);
    state.gradeValue = WideToUtf8(snapshot.gradeValue);
    state.rankingValue = WideToUtf8(snapshot.rankingValue);
    state.contributionValue = WideToUtf8(snapshot.contributionValue);
    state.promotionText = WideToUtf8(snapshot.promotionText);
    for (const auto& line : snapshot.infoLines)
        state.infoLines.push_back(WideToUtf8(line));

    const bool changed = panel.ApplyState(state);
    lastFingerprint_ = fingerprint;
    hasFingerprint_ = true;
    return changed;
#else
    (void)legacy;
    (void)panel;
    (void)viewportWidth;
    (void)viewportHeight;
    return false;
#endif
}

void RmlGensRankingLegacyBridge::Reset() noexcept
{
    lastFingerprint_ = 0;
    hasFingerprint_ = false;
}

} // namespace UI::Modern::PC::Gens
