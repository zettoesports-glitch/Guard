#pragma once
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Combat
{
class RmlDuelConfirmPanel
{
public:
    enum class Action { Accept, Cancel };
    struct State
    {
        bool visible=false;
        std::string title="Duel";
        std::string info;
        std::string acceptLabel="OK";
        std::string cancelLabel="Cancel";
    };

    RmlDuelConfirmPanel();
    ~RmlDuelConfirmPanel();
    RmlDuelConfirmPanel(const RmlDuelConfirmPanel&)=delete;
    RmlDuelConfirmPanel& operator=(const RmlDuelConfirmPanel&)=delete;
    RmlDuelConfirmPanel(RmlDuelConfirmPanel&&) noexcept;
    RmlDuelConfirmPanel& operator=(RmlDuelConfirmPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show=false);
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
}
