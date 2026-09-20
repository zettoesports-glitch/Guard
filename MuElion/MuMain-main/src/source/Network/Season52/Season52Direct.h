#pragma once

#include "Network/Season52/Crypto/Season52Crypto.hpp"
#include "Network/Season52/Packet/MuPacketStream.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class Connection;

namespace mu::net::s52 {

bool DirectProtocolEnabled() noexcept;

class DirectSession {
public:
    static DirectSession& Instance();

    // Must be called whenever SocketClient is replaced.
    // gameServer=false: ConnectServer, no SimpleModulus is required.
    // gameServer=true: GameServer, Enc1/Dec2 are loaded and serial is reset.
    bool BeginConnection(bool gameServer);
    void Reset();

    bool IsGameServerConnection() const noexcept { return gameServer_; }
    bool KeysLoaded() const noexcept { return keysLoaded_; }

    bool SendServerList(Connection* connection);
    bool SendServerAddress(Connection* connection, std::uint16_t serverId);
    bool SendCharacterList(Connection* connection, std::uint8_t language);
    bool SendCreateCharacter(Connection* connection,
                             const wchar_t* name,
                             std::uint8_t classId,
                             std::uint8_t skin);
    bool SendDeleteCharacter(Connection* connection,
                             const wchar_t* name,
                             const wchar_t* personalCode);
    bool SendSelectCharacter(Connection* connection, const wchar_t* name);
    bool SendFinishLoading(Connection* connection);
    bool SendMapServerMoveAuth(
        Connection* connection,
        const wchar_t* account,
        const wchar_t* character,
        std::uint32_t authCode1,
        std::uint32_t authCode2,
        std::uint32_t authCode3,
        std::uint32_t authCode4,
        std::uint32_t tickCount,
        const std::uint8_t* version,
        const std::uint8_t* serial);

    bool SendWalk(Connection* connection,
                  std::uint8_t sourceX,
                  std::uint8_t sourceY,
                  std::uint8_t stepCount,
                  std::uint8_t targetRotation,
                  const std::uint8_t* directions,
                  std::size_t directionsSize);
    bool SendInstantMove(Connection* connection,
                         std::uint8_t targetX,
                         std::uint8_t targetY);
    bool SendAnimation(Connection* connection,
                       std::uint8_t rotation,
                       std::uint8_t animationNumber);
    bool SendHit(Connection* connection,
                 std::uint16_t targetId,
                 std::uint8_t attackAnimation,
                 std::uint8_t lookingDirection);
    bool SendTargetedSkill(Connection* connection,
                           std::uint16_t skillId,
                           std::uint16_t targetId);
    bool SendPickupItem(Connection* connection, std::uint16_t itemId);
    bool SendDropItem(Connection* connection,
                      std::uint8_t targetX,
                      std::uint8_t targetY,
                      std::uint8_t itemSlot);

    bool SendItemMove(
        Connection* connection,
        std::uint8_t sourceStorage,
        std::uint8_t sourceIndex,
        std::uint8_t itemTypeLow,
        std::uint8_t itemLevel,
        std::uint8_t durability,
        std::uint8_t option1,
        std::uint8_t excellentOption,
        std::uint8_t splitType,
        std::uint8_t spareBits,
        const std::array<std::uint8_t, 5>& socketOptions,
        std::uint8_t targetStorage,
        std::uint8_t targetIndex);

    bool SendAreaSkill(Connection* connection,
                       std::uint16_t skillId,
                       std::uint8_t targetX,
                       std::uint8_t targetY,
                       std::uint8_t angle,
                       std::uint8_t destination,
                       std::uint8_t targetPosition,
                       std::uint16_t targetId,
                       std::uint8_t skillSerial);
    bool SendTalkNpc(Connection* connection, std::uint16_t npcId);
    bool SendCloseNpc(Connection* connection);
    bool SendConsumeItem(Connection* connection,
                         std::uint8_t itemSlot,
                         std::uint8_t targetSlot,
                         std::uint8_t fruitUsage);
    bool SendBuyItem(Connection* connection, std::uint8_t itemSlot);
    bool SendSellItem(Connection* connection, std::uint8_t itemSlot);
    bool SendRepairItem(Connection* connection,
                        std::uint8_t itemSlot,
                        std::uint8_t addGold);
    bool SendPublicChat(Connection* connection,
                        const wchar_t* character,
                        const wchar_t* text);
    bool SendWhisper(Connection* connection,
                     const wchar_t* target,
                     const wchar_t* text);

    bool SendTradeRequest(Connection* connection, std::uint16_t targetId);
    bool SendTradeResponse(Connection* connection, std::uint8_t accepted);
    bool SendTradeMoney(Connection* connection, std::uint32_t amount);
    bool SendTradeResult(Connection* connection, std::uint8_t accepted);
    bool SendTradeExit(Connection* connection);

    bool SendPartyInvite(Connection* connection, std::uint16_t targetId);
    bool SendPartyResponse(Connection* connection,
                           std::uint8_t accepted,
                           std::uint16_t requesterId);
    bool SendPartyList(Connection* connection);
    bool SendPartyLeave(Connection* connection, std::uint8_t partyIndex);

    bool SendLogin(Connection* connection,
                   const wchar_t* account,
                   const wchar_t* password,
                   const std::uint8_t* version,
                   const std::uint8_t* serial);

    // Accepts either one complete packet or arbitrary TCP chunks. Output
    // contains complete packets suitable for the existing WSclient dispatcher.
    bool ProcessIncoming(const std::uint8_t* data,
                         std::size_t size,
                         std::vector<std::vector<std::uint8_t>>& packets);

private:
    bool EnsureKeysLoaded();
    bool SendPacket(Connection* connection, const std::vector<std::uint8_t>& packet);
    bool SendXorPacket(Connection* connection, std::vector<std::uint8_t> packet);
    bool SendEncryptedPacket(Connection* connection,
                             const std::vector<std::uint8_t>& canonicalPacket);

private:
    Season52Crypto crypto_;
    MuPacketStream stream_;
    bool gameServer_{false};
    bool keysLoaded_{false};
    bool keyLoadAttempted_{false};
};

} // namespace mu::net::s52
