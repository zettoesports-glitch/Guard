#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

#include <memory>

namespace Rml
{
class Element;
class Event;
}

namespace UI::Modern
{

// Reconstructed from the RTTI-exposed RmlMuMovablePanel::Listener and direct
// x64 Debug function-body analysis. Non-virtual method names are semantic
// reconstruction names; event strings, state layout, clamping and CSS writes
// are recovered behavior.
class RmlMuMovablePanel
{
public:
    RmlMuMovablePanel();
    ~RmlMuMovablePanel();

    RmlMuMovablePanel(const RmlMuMovablePanel&) = delete;
    RmlMuMovablePanel& operator=(const RmlMuMovablePanel&) = delete;

    void Bind(Rml::Element* panel, Rml::Element* dragHandle);
    void Unbind();

    void SetMetrics(float containerWidth, float containerHeight,
                    float panelWidth, float panelHeight,
                    float parentOffsetX, float parentOffsetY);
    void SetPosition(float x, float y);

    [[nodiscard]] Rml::Vector2f GetPosition() const noexcept;
    [[nodiscard]] bool IsDragging() const noexcept;
    void CancelDrag() noexcept;
    [[nodiscard]] bool ConsumePositionChanged() noexcept;

private:
    class Listener final : public Rml::EventListener
    {
    public:
        explicit Listener(RmlMuMovablePanel* owner) noexcept : owner_(owner) {}
        ~Listener() override = default;

        void ProcessEvent(Rml::Event& event) override;

    private:
        RmlMuMovablePanel* owner_ = nullptr;
    };

    void OnEvent(Rml::Event& event);
    [[nodiscard]] bool ClampPosition() noexcept;
    void ApplyPosition();

    std::unique_ptr<Listener> listener_;
    Rml::Element* panel_ = nullptr;
    Rml::Element* dragHandle_ = nullptr;

    float containerWidth_ = 0.0f;
    float containerHeight_ = 0.0f;
    float panelWidth_ = 0.0f;
    float panelHeight_ = 0.0f;
    float parentOffsetX_ = 0.0f;
    float parentOffsetY_ = 0.0f;

    float positionX_ = 0.0f;
    float positionY_ = 0.0f;
    float dragOffsetX_ = 0.0f;
    float dragOffsetY_ = 0.0f;

    bool dragging_ = false;
    bool positionChanged_ = false;
};

} // namespace UI::Modern
