#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Help
{

class RmlHelpPanel
{
public:
    static constexpr std::size_t PageCount = 2;

    enum class ActionType
    {
        Close,
        SelectPage,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t index = 0;
    };

    struct Page
    {
        std::string heading;
        std::vector<std::string> rows;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        float x = 20.0f;
        float y = 20.0f;
        std::string title = "Help";
        std::size_t selectedPage = 0;
        std::array<Page, PageCount> pages;
        std::array<std::string, PageCount> tabLabels{
            "Hot Keys", "Chat"};
    };

    RmlHelpPanel();
    ~RmlHelpPanel();

    RmlHelpPanel(const RmlHelpPanel&) = delete;
    RmlHelpPanel& operator=(const RmlHelpPanel&) = delete;
    RmlHelpPanel(RmlHelpPanel&&) noexcept;
    RmlHelpPanel& operator=(RmlHelpPanel&&) noexcept;

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

} // namespace UI::Modern::PC::Help
