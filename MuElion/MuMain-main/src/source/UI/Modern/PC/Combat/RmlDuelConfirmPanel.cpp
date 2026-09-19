#include "stdafx.h"
#include "UI/Modern/PC/Combat/RmlDuelConfirmPanel.h"
#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <utility>

namespace UI::Modern::PC::Combat
{
namespace
{
constexpr const char* kDocumentPath="Data/UI/PC/Combat/duel_confirm.rml";
void SetText(Rml::Element* element,const std::string& text)
{
    if(!element)return;
    element->SetInnerRML("");
    if(Rml::ElementDocument* doc=element->GetOwnerDocument())
        element->AppendChild(doc->CreateTextNode(text));
}
}

class RmlDuelConfirmPanel::Impl
{
public:
    Impl():host_(kDocumentPath){}
    ~Impl(){Release();}

    bool Load(bool show)
    {
        if(host_.IsLoaded())return show?Show():true;
        if(!host_.Load(show))return false;
        document_=host_.GetDocument();
        if(!document_)return false;

        menu_=document_->GetElementById("menu");
        title_=document_->GetElementById("tfTitle-label");
        info_=document_->GetElementById("taDuelInfo-label");
        okLabel_=document_->GetElementById("btnOk-label");
        cancelLabel_=document_->GetElementById("btnCancel-label");
        Rml::Element* ok=document_->GetElementById("btnOk");
        Rml::Element* cancel=document_->GetElementById("btnCancel");
        if(!menu_||!title_||!info_||!okLabel_||!cancelLabel_||!ok||!cancel)
        {Release();return false;}

        ok_.Bind(ok);
        cancel_.Bind(cancel);
        hostVisible_=show;
        return ApplyState(state_);
    }

    bool Show()
    {
        if(!host_.IsLoaded()&&!Load(false))return false;
        hostVisible_=true;
        const bool ok=host_.Show();
        if(ok)(void)ApplyState(state_);
        return ok;
    }
    bool Hide()
    {
        hostVisible_=false;
        if(menu_)menu_->SetProperty("display","none");
        return host_.Hide();
    }
    void Release()
    {
        ok_.Unbind();cancel_.Unbind();
        menu_=title_=info_=okLabel_=cancelLabel_=nullptr;
        document_=nullptr;
        pending_.reset();
        host_.Release();
    }
    bool IsLoaded() const noexcept{return host_.IsLoaded()&&document_!=nullptr;}

    bool ApplyState(const State& state)
    {
        state_=state;
        if(!IsLoaded())return false;
        menu_->SetProperty("display",(hostVisible_&&state_.visible)?"block":"none");
        SetText(title_,state_.title);
        SetText(info_,state_.info);
        SetText(okLabel_,state_.acceptLabel);
        SetText(cancelLabel_,state_.cancelLabel);
        document_->UpdateDocument();
        return true;
    }

    bool Update()
    {
        if(!IsLoaded())return false;
        bool changed=ok_.Update()|cancel_.Update();
        if(ok_.ConsumeClicked()){pending_=Action::Accept;changed=true;}
        if(cancel_.ConsumeClicked()){pending_=Action::Cancel;changed=true;}
        return changed;
    }
    std::optional<Action> ConsumeAction(){return std::exchange(pending_,std::nullopt);}

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_=nullptr;
    Rml::Element* menu_=nullptr;
    Rml::Element* title_=nullptr;
    Rml::Element* info_=nullptr;
    Rml::Element* okLabel_=nullptr;
    Rml::Element* cancelLabel_=nullptr;
    RmlMuButton ok_,cancel_;
    bool hostVisible_=false;
    State state_;
    std::optional<Action> pending_;
};

RmlDuelConfirmPanel::RmlDuelConfirmPanel():m_impl(std::make_unique<Impl>()){}
RmlDuelConfirmPanel::~RmlDuelConfirmPanel()=default;
RmlDuelConfirmPanel::RmlDuelConfirmPanel(RmlDuelConfirmPanel&&) noexcept=default;
RmlDuelConfirmPanel& RmlDuelConfirmPanel::operator=(RmlDuelConfirmPanel&&) noexcept=default;
bool RmlDuelConfirmPanel::Load(bool show){return m_impl&&m_impl->Load(show);}
bool RmlDuelConfirmPanel::Show(){return m_impl&&m_impl->Show();}
bool RmlDuelConfirmPanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlDuelConfirmPanel::Release(){if(m_impl)m_impl->Release();}
bool RmlDuelConfirmPanel::IsLoaded() const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlDuelConfirmPanel::ApplyState(const State& state){return m_impl&&m_impl->ApplyState(state);}
bool RmlDuelConfirmPanel::Update(){return m_impl&&m_impl->Update();}
std::optional<RmlDuelConfirmPanel::Action> RmlDuelConfirmPanel::ConsumeAction(){return m_impl?m_impl->ConsumeAction():std::nullopt;}
}
