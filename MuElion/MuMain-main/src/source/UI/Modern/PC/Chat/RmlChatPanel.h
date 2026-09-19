#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Chat
{

class RmlChatPanel
{
public:
    enum class MessageKind : std::uint8_t
    {
        Normal = 0,
        Whisper = 1,
        System = 2,
        Error = 3,
        Party = 4,
        Guild = 5,
        Union = 6,
        Gm = 7,
        Gens = 8,
    };

    enum class OutgoingMode : std::uint8_t
    {
        Normal,
        Party,
        Guild,
        Gens,
    };

    struct Message
    {
        std::string sender;
        std::string text;
        MessageKind kind = MessageKind::Normal;
    };

    RmlChatPanel();
    ~RmlChatPanel();

    RmlChatPanel(const RmlChatPanel&) = delete;
    RmlChatPanel& operator=(const RmlChatPanel&) = delete;
    RmlChatPanel(RmlChatPanel&&) noexcept;
    RmlChatPanel& operator=(RmlChatPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = true);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();
    [[nodiscard]] bool IsLoaded() const noexcept;

    void SetVisible(bool visible);
    void SetEditing(bool editing);
    void SetViewportHeight(float viewportHeight);

    void ClearMessages();
    void AddMessage(Message message);
    void SetMessages(std::vector<Message> messages);

    void SetBlockedNames(std::vector<std::string> names);
    [[nodiscard]] const std::vector<std::string>& GetBlockedNames() const noexcept;

    // Polls reconstructed controls and refreshes the retained DOM.
    [[nodiscard]] bool Update();

    [[nodiscard]] OutgoingMode GetOutgoingMode() const noexcept;
    [[nodiscard]] bool IsWhisperFilterEnabled() const noexcept;
    [[nodiscard]] bool IsSystemFilterEnabled() const noexcept;
    [[nodiscard]] bool IsWhisperBlocked() const noexcept;
    [[nodiscard]] int GetAlphaPercent() const noexcept;
    [[nodiscard]] std::size_t GetSizeIndex() const noexcept;

    [[nodiscard]] std::optional<std::string> ConsumeBlockRegisterRequest();
    [[nodiscard]] std::optional<std::string> ConsumeBlockDeleteRequest();

    [[nodiscard]] static const char* MessageClassName(MessageKind kind) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Chat
