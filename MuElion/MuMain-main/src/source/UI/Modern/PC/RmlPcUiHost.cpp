#include "stdafx.h"
#include "UI/Modern/PC/RmlPcUiHost.h"

#include "UI/Modern/PC/Character/RmlCharacterFramePanel.h"
#include "UI/Modern/PC/Character/RmlPetFrameLayer.h"
#include "UI/Modern/PC/Character/RmlPetInfoPanel.h"
#include "UI/Modern/PC/Chat/RmlChatPanel.h"
#include "UI/Modern/PC/Command/RmlCommandWindowPanel.h"
#include "UI/Modern/PC/Common/RmlMessageBoxPanel.h"
#include "UI/Modern/PC/Common/RmlTooltipLayer.h"
#include "UI/Modern/PC/Friend/RmlFriendPanel.h"
#include "UI/Modern/PC/HUD/RmlMainFrameLayer.h"
#include "UI/Modern/PC/HUD/RmlMainFrameLegacyBridge.h"
#include "UI/Modern/PC/HUD/RmlMasterTreePanel.h"
#include "UI/Modern/PC/HUD/RmlMoveCommandPanel.h"
#include "UI/Modern/PC/HUD/RmlTopMenuLayer.h"
#include "UI/Modern/PC/Inventory/RmlInventoryPanel.h"
#include "UI/Modern/PC/Inventory/RmlInventoryLegacyBridge.h"
#include "UI/Modern/PC/Inventory/RmlInventoryExtensionPanel.h"
#include "UI/Modern/PC/Inventory/RmlInventoryExtensionLegacyBridge.h"
#include "UI/Modern/PC/Inventory/RmlPrivateStorePanel.h"
#include "UI/Modern/PC/Inventory/RmlPrivateStoreLegacyBridge.h"
#include "UI/Modern/PC/Inventory/RmlStoragePanel.h"
#include "UI/Modern/PC/Inventory/RmlStorageExtensionPanel.h"
#include "UI/Modern/PC/Inventory/RmlStorageSecurityPanel.h"
#include "UI/Modern/PC/Inventory/RmlTradePanel.h"
#include "UI/Modern/PC/Login/RmlLoginPanel.h"
#include "UI/Modern/PC/MuHelper/RmlMuHelperPanel.h"
#include "UI/Modern/PC/MuHelper/RmlMuHelperLegacyBridge.h"
#include "UI/Modern/PC/Option/RmlOptionPanel.h"
#include "UI/Modern/PC/Party/RmlPartyFrameLayer.h"
#include "UI/Modern/PC/ServerMessage/RmlServerMessageLayer.h"
#include "UI/Modern/PC/SystemMenu/RmlSystemMenuPanel.h"
#include "UI/Modern/PC/World/RmlStoreLabelLayer.h"

#include <utility>

namespace UI::Modern::PC
{

class RmlPcUiHost::Impl
{
public:
    [[nodiscard]] bool Initialize(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return false;

        viewportWidth_ = width;
        viewportHeight_ = height;
        login_.SetViewport(width, height);
        initialized_ = true;
        return true;
    }

    void Shutdown() noexcept
    {
        // Release document owners before RmlUiRuntime shuts its Context down.
        // Reverse roughly follows UI dependency/overlay order.
        storeLabel_.Release();
        serverMessage_.Release();
        tooltip_.Release();
        messageBox_.Release();

        trade_.Release();
        storageSecurity_.Release();
        storageExtension_.Release();
        storage_.Release();
        privateStore_.Release();
        inventoryExtension_.Release();
        inventory_.Release();
        muHelper_.Release();
        option_.Release();
        systemMenu_.Release();
        masterTree_.Release();
        moveCommand_.Release();
        commandWindow_.Release();
        friend_.Release();
        petInfo_.Release();
        petFrame_.Release();
        characterFrame_.Release();
        party_.Release();
        topMenu_.Release();
        mainFrame_.Release();
        chat_.Release();
        login_.Release();

        initialized_ = false;
        viewportWidth_ = 0;
        viewportHeight_ = 0;
    }

    void Resize(int width, int height) noexcept
    {
        if (width <= 0 || height <= 0)
            return;
        viewportWidth_ = width;
        viewportHeight_ = height;

        // State-driven panels receive viewport values through ApplyState.
        // Login owns an explicit viewport contract, so keep it synchronized
        // directly even when it is not yet loaded.
        login_.SetViewport(width, height);
    }

    [[nodiscard]] bool Update() noexcept
    {
        if (!initialized_)
            return false;

        bool changed = false;

        // Only loaded panels are synchronized/polled. Merely initializing
        // the host never creates documents or changes the legacy surface.
        if (mainFrame_.IsLoaded())
        {
            changed |= mainFrameLegacyBridge_.Synchronize(mainFrame_);
            changed |= mainFrame_.Update();
        }
        if (topMenu_.IsLoaded()) changed |= topMenu_.Update();
        if (party_.IsLoaded()) changed |= party_.Update();
        if (chat_.IsLoaded()) changed |= chat_.Update();

        if (characterFrame_.IsLoaded()) changed |= characterFrame_.Update();
        if (petFrame_.IsLoaded()) changed |= petFrame_.Update();
        if (petInfo_.IsLoaded()) changed |= petInfo_.Update();
        if (friend_.IsLoaded()) changed |= friend_.Update();
        if (commandWindow_.IsLoaded()) changed |= commandWindow_.Update();

        if (moveCommand_.IsLoaded()) changed |= moveCommand_.Update();
        if (masterTree_.IsLoaded()) changed |= masterTree_.Update();
        if (option_.IsLoaded()) changed |= option_.Update();
        if (systemMenu_.IsLoaded()) changed |= systemMenu_.Update();

        if (inventory_.IsLoaded())
        {
            changed |= inventoryLegacyBridge_.Synchronize(inventory_);
            changed |= inventory_.Update();
        }
        if (inventoryExtension_.IsLoaded())
        {
            changed |= inventoryExtensionLegacyBridge_.Synchronize(
                inventoryExtension_);
            changed |= inventoryExtension_.Update();
        }
        if (privateStore_.IsLoaded())
        {
            changed |= privateStoreLegacyBridge_.Synchronize(privateStore_);
            changed |= privateStore_.Update();
        }
        if (storage_.IsLoaded()) changed |= storage_.Update();
        if (storageExtension_.IsLoaded()) changed |= storageExtension_.Update();
        if (storageSecurity_.IsLoaded()) changed |= storageSecurity_.Update();
        if (trade_.IsLoaded()) changed |= trade_.Update();
        if (muHelper_.IsLoaded()) changed |= muHelper_.Update();
        if (login_.IsLoaded()) changed |= login_.Update();

        if (messageBox_.IsLoaded()) changed |= messageBox_.Update();

        return changed;
    }

    [[nodiscard]] bool IsInitialized() const noexcept
    {
        return initialized_;
    }

    Character::RmlCharacterFramePanel characterFrame_;
    Character::RmlPetFrameLayer petFrame_;
    Character::RmlPetInfoPanel petInfo_;
    Chat::RmlChatPanel chat_;
    Command::RmlCommandWindowPanel commandWindow_;
    Common::RmlMessageBoxPanel messageBox_;
    Common::RmlTooltipLayer tooltip_;
    Friend::RmlFriendPanel friend_;
    HUD::RmlMainFrameLayer mainFrame_;
    HUD::RmlMainFrameLegacyBridge mainFrameLegacyBridge_;
    HUD::RmlMasterTreePanel masterTree_;
    HUD::RmlMoveCommandPanel moveCommand_;
    HUD::RmlTopMenuLayer topMenu_;
    Inventory::RmlInventoryPanel inventory_;
    Inventory::RmlInventoryLegacyBridge inventoryLegacyBridge_;
    Inventory::RmlInventoryExtensionPanel inventoryExtension_;
    Inventory::RmlInventoryExtensionLegacyBridge
        inventoryExtensionLegacyBridge_;
    Inventory::RmlPrivateStorePanel privateStore_;
    Inventory::RmlStoragePanel storage_;
    Inventory::RmlStorageExtensionPanel storageExtension_;
    Inventory::RmlStorageSecurityPanel storageSecurity_;
    Inventory::RmlTradePanel trade_;
    Login::RmlLoginPanel login_;
    MuHelper::RmlMuHelperPanel muHelper_;
    MuHelper::RmlMuHelperLegacyBridge muHelperLegacyBridge_;
    Option::RmlOptionPanel option_;
    Party::RmlPartyFrameLayer party_;
    ServerMessage::RmlServerMessageLayer serverMessage_;
    SystemMenu::RmlSystemMenuPanel systemMenu_;
    World::RmlStoreLabelLayer storeLabel_;

    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    bool initialized_ = false;
};

RmlPcUiHost::RmlPcUiHost()
    : m_impl(std::make_unique<Impl>())
{
}

RmlPcUiHost::~RmlPcUiHost() = default;

bool RmlPcUiHost::Initialize(int viewportWidth, int viewportHeight)
{
    return m_impl && m_impl->Initialize(viewportWidth, viewportHeight);
}

void RmlPcUiHost::Shutdown() noexcept
{
    if (m_impl)
        m_impl->Shutdown();
}

void RmlPcUiHost::Resize(int viewportWidth, int viewportHeight) noexcept
{
    if (m_impl)
        m_impl->Resize(viewportWidth, viewportHeight);
}

bool RmlPcUiHost::Update() noexcept
{
    return m_impl && m_impl->Update();
}

bool RmlPcUiHost::IsInitialized() const noexcept
{
    return m_impl && m_impl->IsInitialized();
}

#define MU_PC_UI_GETTER(Method, Member, Type) \
Type& RmlPcUiHost::Method() noexcept \
{ \
    return m_impl->Member; \
}

MU_PC_UI_GETTER(CharacterFrame, characterFrame_, Character::RmlCharacterFramePanel)
MU_PC_UI_GETTER(PetFrame, petFrame_, Character::RmlPetFrameLayer)
MU_PC_UI_GETTER(PetInfo, petInfo_, Character::RmlPetInfoPanel)
MU_PC_UI_GETTER(Chat, chat_, Chat::RmlChatPanel)
MU_PC_UI_GETTER(CommandWindow, commandWindow_, Command::RmlCommandWindowPanel)
MU_PC_UI_GETTER(MessageBox, messageBox_, Common::RmlMessageBoxPanel)
MU_PC_UI_GETTER(Tooltip, tooltip_, Common::RmlTooltipLayer)
MU_PC_UI_GETTER(Friend, friend_, Friend::RmlFriendPanel)
MU_PC_UI_GETTER(MainFrame, mainFrame_, HUD::RmlMainFrameLayer)
MU_PC_UI_GETTER(MasterTree, masterTree_, HUD::RmlMasterTreePanel)
MU_PC_UI_GETTER(MoveCommand, moveCommand_, HUD::RmlMoveCommandPanel)
MU_PC_UI_GETTER(TopMenu, topMenu_, HUD::RmlTopMenuLayer)
MU_PC_UI_GETTER(Inventory, inventory_, Inventory::RmlInventoryPanel)
MU_PC_UI_GETTER(InventoryExtension, inventoryExtension_, Inventory::RmlInventoryExtensionPanel)
MU_PC_UI_GETTER(PrivateStore, privateStore_, Inventory::RmlPrivateStorePanel)
MU_PC_UI_GETTER(Storage, storage_, Inventory::RmlStoragePanel)
MU_PC_UI_GETTER(StorageExtension, storageExtension_, Inventory::RmlStorageExtensionPanel)
MU_PC_UI_GETTER(StorageSecurity, storageSecurity_, Inventory::RmlStorageSecurityPanel)
MU_PC_UI_GETTER(Trade, trade_, Inventory::RmlTradePanel)
MU_PC_UI_GETTER(Login, login_, Login::RmlLoginPanel)
MU_PC_UI_GETTER(MuHelper, muHelper_, MuHelper::RmlMuHelperPanel)
MU_PC_UI_GETTER(Option, option_, Option::RmlOptionPanel)
MU_PC_UI_GETTER(Party, party_, Party::RmlPartyFrameLayer)
MU_PC_UI_GETTER(ServerMessage, serverMessage_, ServerMessage::RmlServerMessageLayer)
MU_PC_UI_GETTER(SystemMenu, systemMenu_, SystemMenu::RmlSystemMenuPanel)
MU_PC_UI_GETTER(StoreLabel, storeLabel_, World::RmlStoreLabelLayer)

#undef MU_PC_UI_GETTER

RmlPcUiHost& GetRmlPcUiHost()
{
    static RmlPcUiHost host;
    return host;
}

} // namespace UI::Modern::PC
