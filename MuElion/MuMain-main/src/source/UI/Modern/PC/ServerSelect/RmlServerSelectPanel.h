#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::ServerSelect
{

class RmlServerSelectPanel
{
public:
    enum class ServerKind : unsigned char
    {
        Pvp = 0,
        NonPvp = 1,
        GoldPvp = 2,
        Gold = 3,
    };

    struct Server
    {
        std::string name;
        int connectIndex = -1;
        int serverIndex = -1;
        int loadPercent = 0;
        ServerKind kind = ServerKind::Pvp;
        bool enabled = true;
    };

    struct Group
    {
        int key = -1;
        std::string name;
        std::string description;
        std::vector<Server> servers;
    };

    struct State
    {
        std::string title = "Server Selection";
        std::string advisory;
        std::vector<Group> groups;
        int selectedGroupKey = -1;
    };

    struct Selection
    {
        int groupKey = -1;
        int connectIndex = -1;
        int serverIndex = -1;
        ServerKind kind = ServerKind::Pvp;
        std::string name;
    };

    RmlServerSelectPanel();
    ~RmlServerSelectPanel();

    RmlServerSelectPanel(const RmlServerSelectPanel&) = delete;
    RmlServerSelectPanel& operator=(const RmlServerSelectPanel&) = delete;
    RmlServerSelectPanel(RmlServerSelectPanel&&) noexcept;
    RmlServerSelectPanel& operator=(RmlServerSelectPanel&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    void SetViewport(int width, int height) noexcept;
    [[nodiscard]] bool ApplyState(State state);
    [[nodiscard]] bool Update();

    [[nodiscard]] std::optional<Selection> ConsumeSelection();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::ServerSelect
