#pragma once

#include <cstdint>

namespace SEASON3B
{
class CNewUIChatLogWindow;
struct ChatLogSnapshot;
}

namespace UI::Modern::PC::Chat
{
class RmlChatPanel;

// Adapter boundary between the legacy chat storage and the reconstructed RmlUi
// presentation. It never mutates or owns legacy messages.
class RmlChatLegacyBridge
{
public:
    [[nodiscard]] bool Synchronize(
        const SEASON3B::CNewUIChatLogWindow& legacy,
        RmlChatPanel& panel);

    void Reset() noexcept;

private:
    std::uint64_t lastFingerprint_ = 0;
    bool hasFingerprint_ = false;
};

} // namespace UI::Modern::PC::Chat
