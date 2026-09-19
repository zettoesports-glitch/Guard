#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Command
{

class RmlQuickCommandPanel
{
public:
    enum class Action { Trade = 0, Purchase, Party, Follow, Duel };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        float x = 0.0f;
        float y = 0.0f;
        std::string targetName;
        std::array<bool, 5> enabled{true, true, true, true, true};
        std::array<std::string, 5> labels{
            "Trade", "Purchase", "Party", "Follow", "Duel"};
    };

    RmlQuickCommandPanel();
    ~RmlQuickCommandPanel();
    RmlQuickCommandPanel(const RmlQuickCommandPanel&) = delete;
    RmlQuickCommandPanel& operator=(const RmlQuickCommandPanel&) = delete;
    RmlQuickCommandPanel(RmlQuickCommandPanel&&) noexcept;
    RmlQuickCommandPanel& operator=(RmlQuickCommandPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(State state);
    [[nodiscard]] bool Update();
    [[nodiscard]] std::optional<Action> ConsumeAction();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::Command
