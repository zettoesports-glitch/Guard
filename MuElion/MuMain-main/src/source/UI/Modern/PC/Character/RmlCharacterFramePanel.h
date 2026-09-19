#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace UI::Modern::PC::Character
{

class RmlCharacterFramePanel
{
public:
    enum class ActionType
    {
        IncreaseStat,
        OpenPet,
        OpenMaster,
        Close,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        std::size_t statIndex = 0;
    };

    struct Stat
    {
        std::string label;
        std::string value;
        std::string detail;
        bool visible = true;
        bool canIncrease = false;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string title;
        std::string levelLabel = "Level";
        std::string level;
        std::string classLabel = "Class";
        std::string characterClass;
        std::string serverLabel = "Server";
        std::string server;
        std::string experience;
        std::string pointsLabel = "Points";
        std::string points;

        std::array<Stat, 5> stats{};

        bool petEnabled = true;
        bool masterEnabled = false;
        std::string petLabel = "Pet";
        std::string masterLabel = "Master";
    };

    RmlCharacterFramePanel();
    ~RmlCharacterFramePanel();

    RmlCharacterFramePanel(const RmlCharacterFramePanel&) = delete;
    RmlCharacterFramePanel& operator=(const RmlCharacterFramePanel&) = delete;
    RmlCharacterFramePanel(RmlCharacterFramePanel&&) noexcept;
    RmlCharacterFramePanel& operator=(RmlCharacterFramePanel&&) noexcept;

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

} // namespace UI::Modern::PC::Character
