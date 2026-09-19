#pragma once

#include <memory>
#include <string>
#include <vector>

namespace UI::Modern::PC::World
{

class RmlStoreLabelLayer
{
public:
    enum class Reputation
    {
        Normal,
        Hero,
        Caution,
        MurdererOne,
        MurdererTwo,
        GameMaster,
    };

    enum class GuildRelation
    {
        Neutral,
        Friendly,
        Hostile,
    };

    struct Label
    {
        bool visible = true;
        float screenX = 0.0f;
        float screenY = 0.0f;

        std::string playerId;
        std::string guild;
        std::string title;

        bool storeOpen = false;
        std::string storeTag;
        std::string storeName;
        std::string storeTitle;

        std::string chat;
        bool highlighted = false;
        Reputation reputation = Reputation::Normal;
        GuildRelation guildRelation = GuildRelation::Neutral;

        int gensRank = 0;
        int castleMark = 0;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        std::vector<Label> labels;
    };

    RmlStoreLabelLayer();
    ~RmlStoreLabelLayer();

    RmlStoreLabelLayer(const RmlStoreLabelLayer&) = delete;
    RmlStoreLabelLayer& operator=(const RmlStoreLabelLayer&) = delete;
    RmlStoreLabelLayer(RmlStoreLabelLayer&&) noexcept;
    RmlStoreLabelLayer& operator=(RmlStoreLabelLayer&&) noexcept;

    [[nodiscard]] bool Load(bool show = false);
    [[nodiscard]] bool Show();
    [[nodiscard]] bool Hide();
    void Release();

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool ApplyState(const State& state);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI::Modern::PC::World
