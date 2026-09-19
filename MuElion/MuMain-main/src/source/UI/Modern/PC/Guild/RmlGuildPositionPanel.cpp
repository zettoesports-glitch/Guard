#include "stdafx.h"
#include "UI/Modern/PC/Guild/RmlGuildPositionPanel.h"

#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"
#include "UI/Modern/RmlUiDesign.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>

#include <array>
#include <sstream>
#include <utility>

namespace UI::Modern::PC::Guild
{
namespace
{
constexpr const char* kDocumentPath = "Data/UI/PC/Guild/guild_position.rml";

Rml::Element* ById(Rml::ElementDocument* document, const char* id)
{
    return document ? document->GetElementById(id) : nullptr;
}
void SetText(Rml::Element* element, const std::string& text)
{
    if (!element) return;
    element->SetInnerRML("");
    if (auto* doc = element->GetOwnerDocument())
        element->AppendChild(doc->CreateTextNode(text));
}
void SetVisible(Rml::Element* element, bool visible)
{
    if (element)
        element->SetProperty("display", visible ? "block" : "none");
}
bool ParsePair(
    const RmlUiDesign::Values& values, const char* key,
    std::array<float, 2>& out)
{
    const auto it=values.find(key);
    if(it==values.end()) return false;
    std::istringstream stream(it->second);
    return static_cast<bool>(stream>>out[0]>>out[1]);
}
RmlUiDesign::Values Design()
{
    auto* files=Rml::GetFileInterface();
    if(!files) return {};
    Rml::String text;
    if(!files->LoadFile(kDocumentPath,text)||text.empty()) return {};
    Rml::StreamMemory stream(
        reinterpret_cast<const Rml::byte*>(text.data()),text.size());
    return RmlUiDesign::Parse(&stream);
}
} // namespace

class RmlGuildPositionPanel::Impl
{
public:
    Impl():host_(kDocumentPath){}
    ~Impl(){Release();}

    bool Load(bool show)
    {
        if(host_.IsLoaded()) return show?Show():true;
        if(!host_.Load(show)) return false;
        document_=host_.GetDocument();
        if(!document_) return false;
        const auto d=Design();
        (void)ParsePair(d,"Menu-Size",menuSize_);
        (void)ParsePair(d,"Menu-Reference",reference_);
        (void)ParsePair(d,"Menu-Initial",initial_);
        if(!Bind()){Release();return false;}
        hostVisible_=show;
        return ApplyState(state_);
    }
    bool Show()
    {
        if(!host_.IsLoaded()&&!Load(false)) return false;
        hostVisible_=true;
        const bool ok=host_.Show();
        if(ok) (void)ApplyState(state_);
        return ok;
    }
    bool Hide()
    {
        hostVisible_=false;
        SetVisible(menu_,false);
        return host_.Hide();
    }
    void Release()
    {
        type1_.Unbind(); type2_.Unbind(); ok_.Unbind(); cancel_.Unbind();
        document_=nullptr; menu_=title_=prompt_=nullptr;
        type1Label_=type2Label_=okLabel_=cancelLabel_=nullptr;
        pending_.reset(); host_.Release();
    }
    bool IsLoaded()const noexcept{return host_.IsLoaded()&&document_;}

    bool ApplyState(const State& state)
    {
        state_=state;
        if(state_.selectedType<0||state_.selectedType>2) state_.selectedType=0;
        if(!IsLoaded()) return false;
        const bool visible=hostVisible_&&state_.visible&&
            state_.viewportWidth>0&&state_.viewportHeight>0;
        SetVisible(menu_,visible);
        if(!visible) return true;
        SetText(title_,state_.title);
        SetText(prompt_,state_.prompt);
        SetText(type1Label_,state_.type1Label);
        SetText(type2Label_,state_.type2Label);
        SetText(okLabel_,state_.okLabel);
        SetText(cancelLabel_,state_.cancelLabel);
        if(auto* e=ById(document_,"btnType1"))
            e->SetClass("selected",state_.selectedType==1);
        if(auto* e=ById(document_,"btnType2"))
            e->SetClass("selected",state_.selectedType==2);
        menu_->SetProperty("left",Rml::String(std::to_string(state_.x)+"px"));
        menu_->SetProperty("top",Rml::String(std::to_string(state_.y)+"px"));
        document_->UpdateDocument();
        return true;
    }

    bool Update()
    {
        if(!IsLoaded()) return false;
        bool changed=type1_.Update()|type2_.Update()|ok_.Update()|cancel_.Update();
        if(type1_.ConsumeClicked())
        {
            state_.selectedType=1;
            pending_=Action{ActionType::SelectType,1};
            (void)ApplyState(state_); changed=true;
        }
        if(type2_.ConsumeClicked())
        {
            state_.selectedType=2;
            pending_=Action{ActionType::SelectType,2};
            (void)ApplyState(state_); changed=true;
        }
        if(ok_.ConsumeClicked())
        {
            pending_=Action{ActionType::Confirm,state_.selectedType};
            changed=true;
        }
        if(cancel_.ConsumeClicked())
        {
            pending_=Action{ActionType::Cancel,state_.selectedType};
            changed=true;
        }
        return changed;
    }

    std::optional<Action> ConsumeAction()
    {
        return std::exchange(pending_,std::nullopt);
    }

private:
    bool Bind()
    {
        menu_=ById(document_,"menu");
        title_=ById(document_,"tfTitle");
        prompt_=ById(document_,"taPosMent");
        auto* b1=ById(document_,"btnType1");
        auto* b2=ById(document_,"btnType2");
        auto* bok=ById(document_,"btnOk");
        auto* bc=ById(document_,"btnCancel");
        type1Label_=ById(document_,"btnType1-label");
        type2Label_=ById(document_,"btnType2-label");
        okLabel_=ById(document_,"btnOk-label");
        cancelLabel_=ById(document_,"btnCancel-label");
        if(!menu_||!title_||!prompt_||!b1||!b2||!bok||!bc||
           !type1Label_||!type2Label_||!okLabel_||!cancelLabel_) return false;
        type1_.Bind(b1); type2_.Bind(b2); ok_.Bind(bok); cancel_.Bind(bc);
        return true;
    }

    RmlDocumentHost host_;
    Rml::ElementDocument* document_=nullptr;
    bool hostVisible_=false;
    Rml::Element* menu_=nullptr;
    Rml::Element* title_=nullptr;
    Rml::Element* prompt_=nullptr;
    Rml::Element* type1Label_=nullptr;
    Rml::Element* type2Label_=nullptr;
    Rml::Element* okLabel_=nullptr;
    Rml::Element* cancelLabel_=nullptr;
    RmlMuButton type1_,type2_,ok_,cancel_;
    std::array<float,2> menuSize_{310.001495f,290.000336f};
    std::array<float,2> reference_{640.0f,480.0f};
    std::array<float,2> initial_{20.0f,20.0f};
    State state_;
    std::optional<Action> pending_;
};

RmlGuildPositionPanel::RmlGuildPositionPanel():m_impl(std::make_unique<Impl>()){}
RmlGuildPositionPanel::~RmlGuildPositionPanel()=default;
RmlGuildPositionPanel::RmlGuildPositionPanel(RmlGuildPositionPanel&&) noexcept=default;
RmlGuildPositionPanel& RmlGuildPositionPanel::operator=(RmlGuildPositionPanel&&) noexcept=default;
bool RmlGuildPositionPanel::Load(bool s){return m_impl&&m_impl->Load(s);}
bool RmlGuildPositionPanel::Show(){return m_impl&&m_impl->Show();}
bool RmlGuildPositionPanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlGuildPositionPanel::Release(){if(m_impl)m_impl->Release();}
bool RmlGuildPositionPanel::IsLoaded()const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlGuildPositionPanel::ApplyState(const State&s){return m_impl&&m_impl->ApplyState(s);}
bool RmlGuildPositionPanel::Update(){return m_impl&&m_impl->Update();}
std::optional<RmlGuildPositionPanel::Action> RmlGuildPositionPanel::ConsumeAction()
{return m_impl?m_impl->ConsumeAction():std::nullopt;}

} // namespace UI::Modern::PC::Guild
