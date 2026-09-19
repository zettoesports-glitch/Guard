#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace UI::Modern::PC::Option
{

class RmlOptionPanel
{
public:
    enum class ActionType
    {
        AutomaticAttack,
        WhisperSound,
        NameDisplay,
        SlideHelp,
        RenderAllEffects,
        WindowedMode,
        SoundVolume,
        MusicVolume,
        EffectLevel,
        Font,
        Language,
        Resolution,
        Close,
    };

    struct Action
    {
        ActionType type = ActionType::Close;
        int value = 0;
    };

    struct State
    {
        bool visible = false;
        int viewportWidth = 0;
        int viewportHeight = 0;

        std::string title = "Option";
        std::string automaticLabel = "Automatic Attack";
        std::string whisperLabel = "Whisper Sound";
        std::string nameDisplayLabel = "Name Display";
        std::string soundLabel = "Sound Volume";
        std::string musicLabel = "Music Volume";
        std::string slideLabel = "Slide Help";
        std::string effectLabel = "Effect Limitation";
        std::string renderLabel = "Render All Effects";
        std::string fontLabel = "Font";
        std::string languageLabel = "Language";
        std::string resolutionLabel = "Resolution";
        std::string windowedLabel = "Windowed Mode";
        std::string closeLabel = "Close";

        bool automaticAttack = true;
        bool whisperSound = false;
        bool nameDisplay = true;
        bool slideHelp = true;
        bool renderAllEffects = true;
        bool windowedMode = true;

        int soundVolume = 10;
        int musicVolume = 10;
        int effectLevel = 4;

        std::vector<std::string> fonts;
        std::vector<std::string> languages;
        std::vector<std::string> resolutions;
        int fontIndex = 0;
        int languageIndex = 0;
        int resolutionIndex = 0;
    };

    RmlOptionPanel();
    ~RmlOptionPanel();

    RmlOptionPanel(const RmlOptionPanel&) = delete;
    RmlOptionPanel& operator=(const RmlOptionPanel&) = delete;
    RmlOptionPanel(RmlOptionPanel&&) noexcept;
    RmlOptionPanel& operator=(RmlOptionPanel&&) noexcept;

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

} // namespace UI::Modern::PC::Option
