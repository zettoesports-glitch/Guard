#include "stdafx.h"
#include "UI/Modern/PC/Help/RmlLongNoticeLegacyBridge.h"

#include "UI/Modern/PC/Help/RmlLongNoticeLayer.h"
#include "UI/NewUI/Widgets/NewUISlideWindow.h"

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

void HashWide(std::uint64_t& hash, const std::wstring& text) noexcept
{
    for (wchar_t ch : text)
    {
        const std::uint32_t raw = static_cast<std::uint32_t>(ch);
        HashByte(hash, static_cast<std::uint8_t>(raw));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 8u));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 16u));
        HashByte(hash, static_cast<std::uint8_t>(raw >> 24u));
    }
}

void HashFloat(std::uint64_t& hash, float value) noexcept
{
    std::uint32_t raw = 0;
    static_assert(sizeof(raw) == sizeof(value));
    std::memcpy(&raw, &value, sizeof(raw));
    for (int shift = 0; shift < 32; shift += 8)
        HashByte(hash, static_cast<std::uint8_t>(raw >> shift));
}

std::uint64_t Fingerprint(
    const SlideHelpManagerSnapshot& snapshot,
    int viewportWidth,
    int viewportHeight) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    for (const SlideRenderSnapshot* slide :
         {&snapshot.help, &snapshot.notice})
    {
        HashWide(hash, slide->text);
        HashFloat(hash, slide->referenceX);
        HashFloat(hash, slide->referenceY);
        HashFloat(hash, slide->alphaRate);
        HashByte(hash, slide->textAlpha);
        for (int shift = 0; shift < 32; shift += 8)
            HashByte(hash, static_cast<std::uint8_t>(
                slide->textColor >> shift));
        HashByte(hash, static_cast<std::uint8_t>(slide->idle));
    }
    for (int shift = 0; shift < 32; shift += 8)
    {
        HashByte(hash, static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(viewportWidth) >> shift));
        HashByte(hash, static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(viewportHeight) >> shift));
    }
    return hash;
}

const SlideRenderSnapshot* SelectRenderable(
    const SlideHelpManagerSnapshot& snapshot,
    bool& notice)
{
    // Mirrors CSlideHelpMgr::Render priority without advancing alpha/speed:
    // an active notice first forces help to fade; once help reaches zero the
    // notice owns the strip. Otherwise help owns it.
    if (!snapshot.notice.idle)
    {
        if (snapshot.help.alphaRate <= 0.0f ||
            snapshot.notice.alphaRate > 0.0f)
        {
            notice = true;
            return &snapshot.notice;
        }
        notice = false;
        return &snapshot.help;
    }

    if (!snapshot.help.idle || snapshot.help.alphaRate > 0.0f)
    {
        notice = false;
        return &snapshot.help;
    }

    if (snapshot.notice.alphaRate > 0.0f)
    {
        notice = true;
        return &snapshot.notice;
    }

    return nullptr;
}
} // namespace

bool RmlLongNoticeLegacyBridge::Synchronize(
    const SEASON3B::CNewUISlideWindow& legacy,
    RmlLongNoticeLayer& layer,
    int viewportWidth,
    int viewportHeight)
{
    SlideHelpManagerSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);

    const std::uint64_t fingerprint =
        Fingerprint(snapshot, viewportWidth, viewportHeight);
    if (hasFingerprint_ && fingerprint == lastFingerprint_)
        return false;

    RmlLongNoticeLayer::State state;
    state.viewportWidth = viewportWidth;
    state.viewportHeight = viewportHeight;

    bool notice = false;
    if (const SlideRenderSnapshot* active =
            SelectRenderable(snapshot, notice))
    {
        state.visible =
            active->alphaRate > 0.0f &&
            !active->text.empty();
        state.text = WideToUtf8(active->text);
        state.referenceX = active->referenceX;
        state.referenceY = active->referenceY;
        state.alphaRate = active->alphaRate;
        state.textAlpha = active->textAlpha;
        state.textColor = active->textColor;
        state.notice = notice;
    }

    const bool changed = layer.ApplyState(state);
    lastFingerprint_ = fingerprint;
    hasFingerprint_ = true;
    return changed;
}

void RmlLongNoticeLegacyBridge::Reset() noexcept
{
    lastFingerprint_ = 0;
    hasFingerprint_ = false;
}

} // namespace UI::Modern::PC::Help
