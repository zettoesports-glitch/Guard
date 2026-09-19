#include "stdafx.h"
#include "UI/Modern/PC/Inventory/RmlStorageSecurityPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Inventory
{

namespace
{
constexpr const char* kKeypadPath =
    "Data/UI/PC/Inventory/storage_keypad.rml";
constexpr const char* kPasswordPath =
    "Data/UI/PC/Inventory/storage_password.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx",
                  static_cast<double>(value));
    return Rml::String(buffer.data());
}

bool ParsePair(const char* path, const char* key, float& a, float& b)
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files)
        return false;
    Rml::String contents;
    if (!files->LoadFile(path, contents) || contents.empty())
        return false;
    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(contents.data()), contents.size());
    const auto values = RmlUiDesign::Parse(&stream);
    const auto it = values.find(key);
    if (it == values.end())
        return false;
    std::istringstream parser(it->second);
    return static_cast<bool>(parser >> a >> b);
}

Rml::Element* Element(Rml::ElementDocument* doc, const char* id)
{
    return doc ? doc->GetElementById(id) : nullptr;
}
} // namespace

class RmlStorageSecurityPanel::Impl
{
public:
    Impl()
        : keypadHost_(kKeypadPath),
          passwordHost_(kPasswordPath)
    {
    }

    ~Impl()
    {
        Release();
    }

    bool Load()
    {
        if (loaded_)
            return true;

        if (!keypadHost_.Load(false) || !passwordHost_.Load(false))
        {
            Release();
            return false;
        }

        keypadDoc_ = keypadHost_.GetDocument();
        passwordDoc_ = passwordHost_.GetDocument();
        if (!keypadDoc_ || !passwordDoc_ || !Bind())
        {
            Release();
            return false;
        }

        (void)ParsePair(kKeypadPath, "Menu-Size",
                        keypadWidth_, keypadHeight_);
        (void)ParsePair(kPasswordPath, "Menu-Size",
                        passwordWidth_, passwordHeight_);

        loaded_ = true;
        return true;
    }

    bool ApplyState(const State& state)
    {
        state_ = state;
        state_.pinLimit = std::clamp(state_.pinLimit, 1, 4);

        if (!Load())
            return false;

        SyncDigitLabels();
        keypadOk_.SetEnabled(state_.submitEnabled);
        passwordOk_.SetEnabled(state_.submitEnabled);

        if (state_.mode == Mode::Keypad)
        {
            SetText(Element(keypadDoc_, "tfNumTitle"), state_.title);
            SetText(Element(keypadDoc_, "taNumMent"), state_.prompt);
            SyncMaskedPin();
        }
        else
        {
            SetText(Element(passwordDoc_, "taInputMent"), state_.prompt);
        }

        ApplyLayout();
        return true;
    }

    bool Show(const State& state)
    {
        if (!ApplyState(state))
            return false;

        state_.visible = true;
        if (state_.mode == Mode::Keypad)
        {
            (void)passwordHost_.Hide();
            return keypadHost_.Show();
        }

        (void)keypadHost_.Hide();
        if (passwordInput_)
        {
            passwordInput_->SetValue("");
            passwordInput_->Focus();
        }
        return passwordHost_.Show();
    }

    bool Hide()
    {
        state_.visible = false;
        pin_.clear();
        if (passwordInput_)
            passwordInput_->SetValue("");
        (void)keypadHost_.Hide();
        (void)passwordHost_.Hide();
        SyncMaskedPin();
        return true;
    }

    void Release()
    {
        for (auto& b : digits_)
            b.Unbind();
        keypadBackspace_.Unbind();
        keypadOk_.Unbind();
        keypadCancel_.Unbind();
        passwordOk_.Unbind();
        passwordCancel_.Unbind();

        keypadDoc_ = nullptr;
        passwordDoc_ = nullptr;
        keypadInput_ = nullptr;
        passwordInput_ = nullptr;
        pending_.reset();
        pin_.clear();
        loaded_ = false;

        keypadHost_.Release();
        passwordHost_.Release();
    }

    bool IsLoaded() const noexcept
    {
        return loaded_ && keypadDoc_ && passwordDoc_;
    }

    bool Update()
    {
        if (!IsLoaded() || !state_.visible)
            return false;

        bool changed = false;

        if (state_.mode == Mode::Keypad)
        {
            for (std::size_t i = 0; i < digits_.size(); ++i)
            {
                changed |= digits_[i].Update();
                if (digits_[i].ConsumeClicked() &&
                    pin_.size() < static_cast<std::size_t>(state_.pinLimit))
                {
                    pin_.push_back(
                        static_cast<char>('0' + state_.digitMapping[i]));
                    SyncMaskedPin();
                    changed = true;
                }
            }

            changed |= keypadBackspace_.Update();
            changed |= keypadOk_.Update();
            changed |= keypadCancel_.Update();

            if (keypadBackspace_.ConsumeClicked() && !pin_.empty())
            {
                pin_.pop_back();
                SyncMaskedPin();
                changed = true;
            }

            if (keypadOk_.ConsumeClicked() &&
                pin_.size() == static_cast<std::size_t>(state_.pinLimit))
            {
                pending_ = Action{ActionType::SubmitPin, pin_};
                changed = true;
            }

            if (keypadCancel_.ConsumeClicked())
            {
                pending_ = Action{ActionType::Cancel, {}};
                changed = true;
            }
        }
        else
        {
            changed |= passwordOk_.Update();
            changed |= passwordCancel_.Update();

            if (passwordOk_.ConsumeClicked())
            {
                pending_ = Action{
                    ActionType::SubmitPassword,
                    passwordInput_ ? passwordInput_->GetValue() : std::string{}};
                changed = true;
            }
            if (passwordCancel_.ConsumeClicked())
            {
                pending_ = Action{ActionType::Cancel, {}};
                changed = true;
            }
        }

        return changed;
    }

    std::optional<Action> ConsumeAction()
    {
        auto result = std::exchange(pending_, std::nullopt);
        if (result && result->type != ActionType::Cancel)
        {
            pin_.clear();
            if (passwordInput_)
                passwordInput_->SetValue("");
            SyncMaskedPin();
        }
        return result;
    }

private:
    static void SetText(Rml::Element* element, const std::string& text)
    {
        if (element)
            element->SetInnerRML(text);
    }

    bool Bind()
    {
        keypadInput_ = dynamic_cast<Rml::ElementFormControlInput*>(
            Element(keypadDoc_, "tfInput"));
        passwordInput_ = dynamic_cast<Rml::ElementFormControlInput*>(
            Element(passwordDoc_, "tiInput"));

        if (!keypadInput_ || !passwordInput_)
            return false;

        keypadInput_->SetAttribute("readonly", "readonly");
        keypadInput_->SetAttribute("maxlength", 4);

        for (std::size_t i = 0; i < digits_.size(); ++i)
        {
            const int label = static_cast<int>((i + 1u) % 10u);
            const std::string id = "btnNum" + std::to_string(label);
            Rml::Element* e = Element(keypadDoc_, id.c_str());
            if (!e)
                return false;
            digitElements_[i] = e;
            digits_[i].Bind(e);
        }

        auto bind = [](RmlMuButton& button, Rml::Element* element) {
            if (!element)
                return false;
            button.Bind(element);
            return true;
        };

        return
            bind(keypadBackspace_, Element(keypadDoc_, "btnBackSpace")) &&
            bind(keypadOk_, Element(keypadDoc_, "btnPassOk")) &&
            bind(keypadCancel_, Element(keypadDoc_, "btnPassCancel")) &&
            bind(passwordOk_, Element(passwordDoc_, "btnInputOk")) &&
            bind(passwordCancel_, Element(passwordDoc_, "btnInputCancel"));
    }

    void SyncDigitLabels()
    {
        for (std::size_t i = 0; i < digitElements_.size(); ++i)
        {
            if (digitElements_[i])
                digitElements_[i]->SetInnerRML(
                    std::to_string(state_.digitMapping[i]));
        }
    }

    void SyncMaskedPin()
    {
        if (!keypadInput_)
            return;
        keypadInput_->SetValue(std::string(pin_.size(), '*'));
    }

    void ApplyLayout()
    {
        const float vw = static_cast<float>(state_.viewportWidth);
        const float vh = static_cast<float>(state_.viewportHeight);
        Center(Element(keypadDoc_, "menu"), vw, vh,
               keypadWidth_, keypadHeight_);
        Center(Element(passwordDoc_, "menu"), vw, vh,
               passwordWidth_, passwordHeight_);
    }

    static void Center(Rml::Element* menu, float vw, float vh,
                       float width, float height)
    {
        if (!menu || vw <= 0.0f || vh <= 0.0f)
            return;
        menu->SetProperty("width", PixelValue(width));
        menu->SetProperty("height", PixelValue(height));
        menu->SetProperty("left", PixelValue(std::max(0.0f, (vw-width)*0.5f)));
        menu->SetProperty("top", PixelValue(std::max(0.0f, (vh-height)*0.5f)));
    }

    RmlDocumentHost keypadHost_;
    RmlDocumentHost passwordHost_;
    Rml::ElementDocument* keypadDoc_ = nullptr;
    Rml::ElementDocument* passwordDoc_ = nullptr;
    Rml::ElementFormControlInput* keypadInput_ = nullptr;
    Rml::ElementFormControlInput* passwordInput_ = nullptr;
    std::array<Rml::Element*, 10> digitElements_{};

    std::array<RmlMuButton, 10> digits_{};
    RmlMuButton keypadBackspace_;
    RmlMuButton keypadOk_;
    RmlMuButton keypadCancel_;
    RmlMuButton passwordOk_;
    RmlMuButton passwordCancel_;

    State state_;
    std::string pin_;
    std::optional<Action> pending_;
    float keypadWidth_ = 310.001495f;
    float keypadHeight_ = 372.999298f;
    float passwordWidth_ = 306.0f;
    float passwordHeight_ = 189.0f;
    bool loaded_ = false;
};

RmlStorageSecurityPanel::RmlStorageSecurityPanel()
    : m_impl(std::make_unique<Impl>()) {}
RmlStorageSecurityPanel::~RmlStorageSecurityPanel() = default;
RmlStorageSecurityPanel::RmlStorageSecurityPanel(
    RmlStorageSecurityPanel&&) noexcept = default;
RmlStorageSecurityPanel& RmlStorageSecurityPanel::operator=(
    RmlStorageSecurityPanel&&) noexcept = default;

bool RmlStorageSecurityPanel::Load() { return m_impl && m_impl->Load(); }
bool RmlStorageSecurityPanel::ApplyState(const State& state) { return m_impl && m_impl->ApplyState(state); }
bool RmlStorageSecurityPanel::Show(const State& state) { return m_impl && m_impl->Show(state); }
bool RmlStorageSecurityPanel::Hide() { return m_impl && m_impl->Hide(); }
void RmlStorageSecurityPanel::Release() { if (m_impl) m_impl->Release(); }
bool RmlStorageSecurityPanel::IsLoaded() const noexcept { return m_impl && m_impl->IsLoaded(); }
bool RmlStorageSecurityPanel::Update() { return m_impl && m_impl->Update(); }
std::optional<RmlStorageSecurityPanel::Action> RmlStorageSecurityPanel::ConsumeAction()
{
    return m_impl ? m_impl->ConsumeAction() : std::nullopt;
}

} // namespace UI::Modern::PC::Inventory
