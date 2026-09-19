#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::HUD
{

class RmlMoveCommandPanel
{
public:
    static constexpr std::size_t MainRowCount = 12;
    static constexpr std::size_t FavoriteRowCount = 5;

    enum class ActionType
    {
        SelectDestination,
        ToggleFavorite,
        ShowMap,
        Close,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        int mapIndex = -1;
    };

    struct Destination
    {
        int mapIndex = -1;
        std::string strife;
        std::string map;
        std::string level;
        std::string zen;
        bool canMove = true;
        bool favorite = false;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string title = "Move";
        std::string strifeLabel = "Battle";
        std::string mapLabel = "Map";
        std::string levelLabel = "Level";
        std::string zenLabel = "Zen";
        std::string favoriteLabel = "Favorite";
        std::string favoriteTitle = "Favorites";
        std::string showMapLabel = "Show Map";
        std::string closeLabel = "Close";

        std::vector<Destination> destinations;
        std::vector<Destination> favorites;
        std::optional<int> selectedMapIndex;
    };

    RmlMoveCommandPanel();
    ~RmlMoveCommandPanel();

    RmlMoveCommandPanel(const RmlMoveCommandPanel&) = delete;
    RmlMoveCommandPanel& operator=(const RmlMoveCommandPanel&) = delete;
    RmlMoveCommandPanel(RmlMoveCommandPanel&&) noexcept;
    RmlMoveCommandPanel& operator=(RmlMoveCommandPanel&&) noexcept;

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

} // namespace UI::Modern::PC::HUD
