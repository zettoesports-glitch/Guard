#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Guild
{

// Presentation-only guild-position modal. Appointment/network ownership stays
// in the legacy guild message-box path.
class RmlGuildPositionPanel
{
public:
    enum class ActionType : std::uint8_t
    {
        SelectType,
        Confirm,
        Cancel,
    };

    struct Action
    {
        ActionType type = ActionType::Cancel;
        int positionType = 0;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        float x = 20.0f;
        float y = 20.0f;
        std::string title = "Guild Position";
        std::string prompt;
        std::string type1Label = "Position 1";
        std::string type2Label = "Position 2";
        std::string okLabel = "OK";
        std::string cancelLabel = "Cancel";
        int selectedType = 0;
    };

    RmlGuildPositionPanel();
    ~RmlGuildPositionPanel();
    RmlGuildPositionPanel(const RmlGuildPositionPanel&) = delete;
    RmlGuildPositionPanel& operator=(const RmlGuildPositionPanel&) = delete;
    RmlGuildPositionPanel(RmlGuildPositionPanel&&) noexcept;
    RmlGuildPositionPanel& operator=(RmlGuildPositionPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);
    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Guild
