#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace mu::net::s52 {

class Season52Crypto;

inline constexpr std::size_t AccountSize = 10;
inline constexpr std::size_t CharacterNameSize = 10;
inline constexpr std::size_t PasswordSize = 20;
inline constexpr std::size_t PersonalCodeSize = 20;
inline constexpr std::size_t ProtocolVersionSize = 5;
inline constexpr std::size_t ProtocolSerialSize = 16;

std::vector<std::uint8_t> BuildServerListRequest();
std::vector<std::uint8_t> BuildServerAddressRequest(std::uint16_t serverId);

std::vector<std::uint8_t> BuildCharacterListRequest(std::uint8_t language);
std::vector<std::uint8_t> BuildCreateCharacterRequest(std::string_view name,
                                                       std::uint8_t classId,
                                                       std::uint8_t skin);
std::vector<std::uint8_t> BuildDeleteCharacterRequest(std::string_view name,
                                                       std::string_view personalCode);
std::vector<std::uint8_t> BuildSelectCharacterRequest(std::string_view name);
std::vector<std::uint8_t> BuildFinishLoadingRequest();
std::vector<std::uint8_t> BuildQuestHistoryRequest();
std::vector<std::uint8_t> BuildQuestStateRequest(std::uint8_t questIndex,
                                                  std::uint8_t questState);

std::vector<std::uint8_t> BuildCloseNpcRequest();
std::vector<std::uint8_t> BuildPublicChatRequest(std::string_view character,
                                                 std::string_view text);
std::vector<std::uint8_t> BuildWhisperRequest(std::string_view target,
                                              std::string_view text);

bool BuildAreaSkillRequest(std::uint16_t skillId,
                           std::uint8_t targetX,
                           std::uint8_t targetY,
                           std::uint8_t angle,
                           std::uint8_t destination,
                           std::uint8_t targetPosition,
                           std::uint16_t targetId,
                           std::uint8_t skillSerial,
                           Season52Crypto& crypto,
                           std::vector<std::uint8_t>& wirePacket);
bool BuildTalkNpcRequest(std::uint16_t npcId,
                         Season52Crypto& crypto,
                         std::vector<std::uint8_t>& wirePacket);
bool BuildConsumeItemRequest(std::uint8_t itemSlot,
                             std::uint8_t targetSlot,
                             std::uint8_t fruitUsage,
                             Season52Crypto& crypto,
                             std::vector<std::uint8_t>& wirePacket);
bool BuildBuyItemRequest(std::uint8_t itemSlot,
                         Season52Crypto& crypto,
                         std::vector<std::uint8_t>& wirePacket);
bool BuildSellItemRequest(std::uint8_t itemSlot,
                          Season52Crypto& crypto,
                          std::vector<std::uint8_t>& wirePacket);
bool BuildRepairItemRequest(std::uint8_t itemSlot,
                            std::uint8_t addGold,
                            Season52Crypto& crypto,
                            std::vector<std::uint8_t>& wirePacket);

bool BuildItemMoveRequest(
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
    std::uint8_t targetIndex,
    Season52Crypto& crypto,
    std::vector<std::uint8_t>& wirePacket);

bool BuildMapServerMoveAuthRequest(
                       std::string_view account,
                       std::string_view character,
                       std::uint32_t authCode1,
                       std::uint32_t authCode2,
                       std::uint32_t authCode3,
                       std::uint32_t authCode4,
                       std::uint32_t tickCount,
                       const std::array<std::uint8_t, ProtocolVersionSize>& version,
                       const std::array<std::uint8_t, ProtocolSerialSize>& serial,
                       Season52Crypto& crypto,
                       std::vector<std::uint8_t>& wirePacket);

bool BuildLoginRequest(std::string_view account,
                       std::string_view password,
                       std::uint32_t tickCount,
                       const std::array<std::uint8_t, ProtocolVersionSize>& version,
                       const std::array<std::uint8_t, ProtocolSerialSize>& serial,
                       Season52Crypto& crypto,
                       std::vector<std::uint8_t>& wirePacket);

} // namespace mu::net::s52
