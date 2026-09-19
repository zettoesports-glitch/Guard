#include "stdafx.h"
#include "UI/Modern/PC/Command/RmlQuickCommandPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <array>
#include <cstdio>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Command
{
namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/Command/quick_command.rml";

Rml::String PixelValue(float value)
{
    std::array<char, 64> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.3fpx", static_cast<double>(value));
    return Rml::String(buffer.data());
}

bool ParsePair(const RmlUiDesign::Values& values, const char* key, std::array<float, 2>& output)
{
    const auto it = values.find(key);
    if (it == values.end()) return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(stream >> output[0] >> output[1]);
}

RmlUiDesign::Values LoadDesignValues()
{
    Rml::FileInterface* files = Rml::GetFileInterface();
    if (!files) return {};
    Rml::String contents;
    if (!files->LoadFile(kDocumentPath, contents) || contents.empty()) return {};
    Rml::StreamMemory stream(reinterpret_cast<const Rml::byte*>(contents.data()), contents.size());
    return RmlUiDesign::Parse(&stream);
}

void SetText(Rml::Element* element, const std::string& text)
{
    if (!element) return;
    element->SetInnerRML("");
    if (Rml::ElementDocument* document = element->GetOwnerDocument())
        element->AppendChild(document->CreateTextNode(text));
}
} // namespace

class RmlQuickCommandPanel::Impl
{
public:
    Impl() : host_(kDocumentPath) {}
    ~Impl() { Release(); }

    bool Load(bool show)
    {
        if (host_.IsLoaded()) return show ? Show() : true;
        if (!host_.Load(show)) return false;
        document_ = host_.GetDocument();
        if (!document_) return false;

        const auto values = LoadDesignValues();
        (void)ParsePair(values, "Menu-Size", menuSize_);

        menu_ = document_->GetElementById("menu");
        title_ = document_->GetElementById("menu-title");
        if (!menu_ || !title_) { Release(); return false; }

        static constexpr const char* ids[5] = {
            "btnTrade","btnPurchase","btnParty","btnFollow","btnDuel"};
        static constexpr const char* labelIds[5] = {
            "btnTrade-label","btnPurchase-label","btnParty-label","btnFollow-label","btnDuel-label"};

        for (std::size_t i=0;i<buttons_.size();++i)
        {
            Rml::Element* element=document_->GetElementById(ids[i]);
            labels_[i]=document_->GetElementById(labelIds[i]);
            if (!element || !labels_[i]) { Release(); return false; }
            buttons_[i].Bind(element);
        }

        hostVisible_=show;
        return ApplyState(state_);
    }

    bool Show()
    {
        if (!host_.IsLoaded() && !Load(false)) return false;
        hostVisible_=true;
        const bool ok=host_.Show();
        if (ok) (void)ApplyState(state_);
        return ok;
    }

    bool Hide()
    {
        hostVisible_=false;
        if (menu_) menu_->SetProperty("display","none");
        return host_.Hide();
    }

    void Release()
    {
        for(auto& button:buttons_) button.Unbind();
        labels_.fill(nullptr);
        menu_=title_=nullptr;
        document_=nullptr;
        pending_.reset();
        host_.Release();
    }

    bool IsLoaded() const noexcept { return host_.IsLoaded() && document_!=nullptr; }

    bool ApplyState(const State& state)
    {
        state_=state;
        if (!IsLoaded()) return false;
        const bool visible=hostVisible_ && state_.visible &&
            state_.viewportWidth>0 && state_.viewportHeight>0;
        menu_->SetProperty("display",visible?"block":"none");
        if (!visible) return true;

        menu_->SetProperty("left",PixelValue(state_.x));
        menu_->SetProperty("top",PixelValue(state_.y));
        menu_->SetProperty("width",PixelValue(menuSize_[0]));
        menu_->SetProperty("height",PixelValue(menuSize_[1]));
        SetText(title_,state_.targetName);

        for(std::size_t i=0;i<buttons_.size();++i)
        {
            buttons_[i].SetEnabled(state_.enabled[i]);
            (void)buttons_[i].Update();
            SetText(labels_[i],state_.labels[i]);
        }
        document_->UpdateDocument();
        return true;
    }

    bool Update()
    {
        if (!IsLoaded()) return false;
        bool changed=false;
        for(std::size_t i=0;i<buttons_.size();++i)
        {
            changed|=buttons_[i].Update();
            if(buttons_[i].ConsumeClicked())
            {
                pending_=static_cast<Action>(i);
                changed=true;
            }
        }
        return changed;
    }

    std::optional<Action> ConsumeAction() { return std::exchange(pending_,std::nullopt); }

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_=nullptr;
    Rml::Element* menu_=nullptr;
    Rml::Element* title_=nullptr;
    std::array<Rml::Element*,5> labels_{};
    std::array<RmlMuButton,5> buttons_{};
    std::array<float,2> menuSize_{145.0f,148.0f};
    bool hostVisible_=false;
    State state_;
    std::optional<Action> pending_;
};

RmlQuickCommandPanel::RmlQuickCommandPanel():m_impl(std::make_unique<Impl>()){}
RmlQuickCommandPanel::~RmlQuickCommandPanel()=default;
RmlQuickCommandPanel::RmlQuickCommandPanel(RmlQuickCommandPanel&&) noexcept=default;
RmlQuickCommandPanel& RmlQuickCommandPanel::operator=(RmlQuickCommandPanel&&) noexcept=default;
bool RmlQuickCommandPanel::Load(bool show){return m_impl&&m_impl->Load(show);}
bool RmlQuickCommandPanel::Show(){return m_impl&&m_impl->Show();}
bool RmlQuickCommandPanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlQuickCommandPanel::Release(){if(m_impl)m_impl->Release();}
bool RmlQuickCommandPanel::IsLoaded() const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlQuickCommandPanel::ApplyState(State state){return m_impl&&m_impl->ApplyState(state);}
bool RmlQuickCommandPanel::Update(){return m_impl&&m_impl->Update();}
std::optional<RmlQuickCommandPanel::Action> RmlQuickCommandPanel::ConsumeAction(){return m_impl?m_impl->ConsumeAction():std::nullopt;}

} // namespace UI::Modern::PC::Command
