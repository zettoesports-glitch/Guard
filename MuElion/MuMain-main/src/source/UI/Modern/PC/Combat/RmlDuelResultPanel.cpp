#include "stdafx.h"
#include "UI/Modern/PC/Combat/RmlDuelResultPanel.h"
#include "UI/Modern/RmlDocumentHost.h"
#include "UI/Modern/RmlMuButton.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <utility>

namespace UI::Modern::PC::Combat
{
namespace
{
constexpr const char* kDocumentPath="Data/UI/PC/Combat/duel_result.rml";
void SetText(Rml::Element* element,const std::string& text)
{
    if(!element)return;
    element->SetInnerRML("");
    if(Rml::ElementDocument* doc=element->GetOwnerDocument())
        element->AppendChild(doc->CreateTextNode(text));
}
}

class RmlDuelResultPanel::Impl
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
        closeLabel_=document_->GetElementById("btnEnd-label");
        Rml::Element* close=document_->GetElementById("btnEnd");
        if(!menu_||!title_||!info_||!closeLabel_||!close)
        {Release();return false;}
        close_.Bind(close);
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
        close_.Unbind();
        menu_=title_=info_=closeLabel_=nullptr;
        document_=nullptr;
        closeRequest_=false;
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
        SetText(closeLabel_,state_.closeLabel);
        document_->UpdateDocument();
        return true;
    }

    bool Update()
    {
        if(!IsLoaded())return false;
        bool changed=close_.Update();
        if(close_.ConsumeClicked()){closeRequest_=true;changed=true;}
        return changed;
    }
    bool ConsumeCloseRequest() noexcept{return std::exchange(closeRequest_,false);}

private:
    RmlDocumentHost host_;
    Rml::ElementDocument* document_=nullptr;
    Rml::Element* menu_=nullptr;
    Rml::Element* title_=nullptr;
    Rml::Element* info_=nullptr;
    Rml::Element* closeLabel_=nullptr;
    RmlMuButton close_;
    bool hostVisible_=false;
    bool closeRequest_=false;
    State state_;
};

RmlDuelResultPanel::RmlDuelResultPanel():m_impl(std::make_unique<Impl>()){}
RmlDuelResultPanel::~RmlDuelResultPanel()=default;
RmlDuelResultPanel::RmlDuelResultPanel(RmlDuelResultPanel&&) noexcept=default;
RmlDuelResultPanel& RmlDuelResultPanel::operator=(RmlDuelResultPanel&&) noexcept=default;
bool RmlDuelResultPanel::Load(bool show){return m_impl&&m_impl->Load(show);}
bool RmlDuelResultPanel::Show(){return m_impl&&m_impl->Show();}
bool RmlDuelResultPanel::Hide(){return m_impl&&m_impl->Hide();}
void RmlDuelResultPanel::Release(){if(m_impl)m_impl->Release();}
bool RmlDuelResultPanel::IsLoaded() const noexcept{return m_impl&&m_impl->IsLoaded();}
bool RmlDuelResultPanel::ApplyState(const State& state){return m_impl&&m_impl->ApplyState(state);}
bool RmlDuelResultPanel::Update(){return m_impl&&m_impl->Update();}
bool RmlDuelResultPanel::ConsumeCloseRequest() noexcept{return m_impl&&m_impl->ConsumeCloseRequest();}
}
