#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Gens
{

class RmlGensRankingPanel
{
public:
    enum class Influence
    {
        None = 0,
        Duprian = 1,
        Vanert = 2,
    };

    enum class Action
    {
        Close,
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;
        float x = 20.0f;
        float y = 20.0f;

        Influence influence = Influence::None;
        int rankIndex = 0;

        std::string title;
        std::string gensInfoLabel;
        std::string gensTeamLabel;
        std::string gensRankingLabel;
        std::string rankingLabel;
        std::string contributionLabel;

        std::string teamValue;
        std::string gradeValue;
        std::string rankingValue;
        std::string contributionValue;
        std::string promotionText;
        std::vector<std::string> infoLines;
    };

    RmlGensRankingPanel();
    ~RmlGensRankingPanel();

    RmlGensRankingPanel(const RmlGensRankingPanel&) = delete;
    RmlGensRankingPanel& operator=(const RmlGensRankingPanel&) = delete;
    RmlGensRankingPanel(RmlGensRankingPanel&&) noexcept;
    RmlGensRankingPanel& operator=(RmlGensRankingPanel&&) noexcept;

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

} // namespace UI::Modern::PC::Gens
