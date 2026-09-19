#pragma once

#include <memory>

namespace UI::Modern::PC
{

namespace Character
{
class RmlCharacterCreatePanel;
class RmlCharacterFramePanel;
class RmlPetFrameLayer;
class RmlPetInfoPanel;
}
namespace Chat { class RmlChatPanel; }
namespace Combat
{
class RmlDuelConfirmPanel;
class RmlDuelResultPanel;
class RmlDuelWatchPanel;
}
namespace Command
{
class RmlCommandWindowPanel;
class RmlQuickCommandPanel;
}
namespace Common
{
class RmlMessageBoxPanel;
class RmlTooltipLayer;
}
namespace Friend { class RmlFriendPanel; }
namespace HUD
{
class RmlBuffListLayer;
class RmlMainFrameLayer;
class RmlMiniMapLayer;
class RmlMasterTreePanel;
class RmlMoveCommandPanel;
class RmlTopMenuLayer;
}
namespace Inventory
{
class RmlInventoryPanel;
class RmlInventoryExtensionPanel;
class RmlPrivateStorePanel;
class RmlStoragePanel;
class RmlStorageExtensionPanel;
class RmlStorageSecurityPanel;
class RmlTradePanel;
}
namespace Login { class RmlLoginPanel; }
namespace MuHelper { class RmlMuHelperPanel; }
namespace Option { class RmlOptionPanel; }
namespace Party { class RmlPartyFrameLayer; }
namespace ServerMessage { class RmlServerMessageLayer; }
namespace ServerSelect { class RmlServerSelectPanel; }
namespace SystemMenu
{
class RmlSystemMenuPanel;
class RmlWindowMenuPanel;
}
namespace World
{
class RmlMapNameLayer;
class RmlMonsterInfoLayer;
class RmlStoreLabelLayer;
}

class RmlPcUiHost
{
public:
    RmlPcUiHost();
    ~RmlPcUiHost();

    RmlPcUiHost(const RmlPcUiHost&) = delete;
    RmlPcUiHost& operator=(const RmlPcUiHost&) = delete;

    [[nodiscard]] bool Initialize(int viewportWidth, int viewportHeight);
    void Shutdown() noexcept;
    void Resize(int viewportWidth, int viewportHeight) noexcept;
    [[nodiscard]] bool Update() noexcept;
    [[nodiscard]] bool IsInitialized() const noexcept;

    [[nodiscard]] Character::RmlCharacterCreatePanel& CharacterCreate() noexcept;
    [[nodiscard]] Character::RmlCharacterFramePanel& CharacterFrame() noexcept;
    [[nodiscard]] Character::RmlPetFrameLayer& PetFrame() noexcept;
    [[nodiscard]] Character::RmlPetInfoPanel& PetInfo() noexcept;
    [[nodiscard]] Chat::RmlChatPanel& Chat() noexcept;
    [[nodiscard]] Combat::RmlDuelConfirmPanel& DuelConfirm() noexcept;
    [[nodiscard]] Combat::RmlDuelResultPanel& DuelResult() noexcept;
    [[nodiscard]] Combat::RmlDuelWatchPanel& DuelWatch() noexcept;
    [[nodiscard]] Command::RmlCommandWindowPanel& CommandWindow() noexcept;
    [[nodiscard]] Command::RmlQuickCommandPanel& QuickCommand() noexcept;
    [[nodiscard]] Common::RmlMessageBoxPanel& MessageBox() noexcept;
    [[nodiscard]] Common::RmlTooltipLayer& Tooltip() noexcept;
    [[nodiscard]] Friend::RmlFriendPanel& Friend() noexcept;
    [[nodiscard]] HUD::RmlBuffListLayer& BuffList() noexcept;
    [[nodiscard]] HUD::RmlMainFrameLayer& MainFrame() noexcept;
    [[nodiscard]] HUD::RmlMiniMapLayer& MiniMap() noexcept;
    [[nodiscard]] HUD::RmlMasterTreePanel& MasterTree() noexcept;
    [[nodiscard]] HUD::RmlMoveCommandPanel& MoveCommand() noexcept;
    [[nodiscard]] HUD::RmlTopMenuLayer& TopMenu() noexcept;
    [[nodiscard]] Inventory::RmlInventoryPanel& Inventory() noexcept;
    [[nodiscard]] Inventory::RmlInventoryExtensionPanel& InventoryExtension() noexcept;
    [[nodiscard]] Inventory::RmlPrivateStorePanel& PrivateStore() noexcept;
    [[nodiscard]] Inventory::RmlStoragePanel& Storage() noexcept;
    [[nodiscard]] Inventory::RmlStorageExtensionPanel& StorageExtension() noexcept;
    [[nodiscard]] Inventory::RmlStorageSecurityPanel& StorageSecurity() noexcept;
    [[nodiscard]] Inventory::RmlTradePanel& Trade() noexcept;
    [[nodiscard]] Login::RmlLoginPanel& Login() noexcept;
    [[nodiscard]] MuHelper::RmlMuHelperPanel& MuHelper() noexcept;
    [[nodiscard]] Option::RmlOptionPanel& Option() noexcept;
    [[nodiscard]] Party::RmlPartyFrameLayer& Party() noexcept;
    [[nodiscard]] ServerMessage::RmlServerMessageLayer& ServerMessage() noexcept;
    [[nodiscard]] ServerSelect::RmlServerSelectPanel& ServerSelect() noexcept;
    [[nodiscard]] SystemMenu::RmlSystemMenuPanel& SystemMenu() noexcept;
    [[nodiscard]] SystemMenu::RmlWindowMenuPanel& WindowMenu() noexcept;
    [[nodiscard]] World::RmlMapNameLayer& MapName() noexcept;
    [[nodiscard]] World::RmlMonsterInfoLayer& MonsterInfo() noexcept;
    [[nodiscard]] World::RmlStoreLabelLayer& StoreLabel() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

RmlPcUiHost& GetRmlPcUiHost();

} // namespace UI::Modern::PC
