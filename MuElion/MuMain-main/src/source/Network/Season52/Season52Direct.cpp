#include "stdafx.h"
#include "Network/Season52/Season52Direct.h"

#include "Core/Utilities/Log/MuLogger.h"
#include "Data/Translation/MultiLanguage.h"
#include "Dotnet/Connection.h"
#include "Network/Season52/Packet/Season52PacketBuilder.hpp"
#include "Network/Season52/Crypto/Season52Xor.hpp"

#include <array>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace mu::net::s52 {
namespace {

constexpr const char* kDirectProtocolEnv = "MU_PROTOCOL_SEASON52_DIRECT";
constexpr const char* kEnc1PathEnv = "MU_S52_ENC1_PATH";
constexpr const char* kDec2PathEnv = "MU_S52_DEC2_PATH";
constexpr const char* kVersionEnv = "MU_S52_VERSION";
constexpr const char* kSerialEnv = "MU_S52_SERIAL";

constexpr const char* kLouisDefaultVersion = "1.04.05";
constexpr const char* kLouisDefaultSerial = "TbYehR2hFUPBKgZj";

bool IsEnabledValue(const char* value) noexcept
{
    return value != nullptr
        && (std::strcmp(value, "1") == 0
            || std::strcmp(value, "true") == 0
            || std::strcmp(value, "TRUE") == 0);
}

bool ResolveProtocolIdentity(
    std::array<std::uint8_t, ProtocolVersionSize>& encodedVersion,
    std::array<std::uint8_t, ProtocolSerialSize>& serial)
{
    const char* configuredVersion = std::getenv(kVersionEnv);
    if (configuredVersion == nullptr || configuredVersion[0] == '\0')
    {
        configuredVersion = kLouisDefaultVersion;
    }

    std::array<std::uint8_t, ProtocolVersionSize> wireVersion{};
    std::size_t versionIndex = 0;
    for (const char* p = configuredVersion;
         *p != '\0' && versionIndex < wireVersion.size();
         ++p)
    {
        if (*p == '.')
        {
            continue;
        }

        if (*p < '0' || *p > '9')
        {
            return false;
        }

        wireVersion[versionIndex++] = static_cast<std::uint8_t>(*p);
    }

    if (versionIndex != wireVersion.size())
    {
        return false;
    }

    // Season52PacketBuilder follows the original Louis Main convention:
    // Version[] is stored offset by its one-based position and the builder
    // subtracts that position before putting the five bytes on the wire.
    for (std::size_t i = 0; i < encodedVersion.size(); ++i)
    {
        encodedVersion[i] =
            static_cast<std::uint8_t>(wireVersion[i] + (i + 1u));
    }

    const char* configuredSerial = std::getenv(kSerialEnv);
    if (configuredSerial == nullptr || configuredSerial[0] == '\0')
    {
        configuredSerial = kLouisDefaultSerial;
    }

    if (std::strlen(configuredSerial) != serial.size())
    {
        return false;
    }

    std::copy_n(
        reinterpret_cast<const std::uint8_t*>(configuredSerial),
        serial.size(),
        serial.begin());
    return true;
}

bool ReadKeyFile(const char* envName,
                 const char* defaultPath,
                 std::array<std::uint8_t, SimpleModulus::KeyFileSize>& bytes,
                 std::string& usedPath)
{
    const char* overridePath = std::getenv(envName);
    usedPath = (overridePath != nullptr && overridePath[0] != '\0')
        ? overridePath
        : defaultPath;

    std::ifstream file(usedPath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return false;
    }

    const auto length = file.tellg();
    if (length != static_cast<std::streamoff>(bytes.size()))
    {
        return false;
    }

    file.seekg(0, std::ios::beg);
    return static_cast<bool>(
        file.read(reinterpret_cast<char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size())));
}

} // namespace

bool DirectProtocolEnabled() noexcept
{
    static const bool enabled = IsEnabledValue(std::getenv(kDirectProtocolEnv));
    return enabled;
}

DirectSession& DirectSession::Instance()
{
    static DirectSession instance;
    return instance;
}

bool DirectSession::BeginConnection(bool gameServer)
{
    stream_.Reset();
    gameServer_ = gameServer;

    if (!gameServer_)
    {
        return true;
    }

    if (!EnsureKeysLoaded())
    {
        mu::log::Get("network")->error(
            "S52: direct GameServer connection requires Data/Enc1.dat and Data/Dec2.dat");
        return false;
    }

    crypto_.ResetSerial(0);
    mu::log::Get("network")->info(
        "S52: direct protocol ready for GameServer (serial reset)");
    return true;
}

void DirectSession::Reset()
{
    stream_.Reset();
    gameServer_ = false;
    crypto_.ResetSerial(0);
}

bool DirectSession::EnsureKeysLoaded()
{
    if (keysLoaded_)
    {
        return true;
    }

    if (keyLoadAttempted_)
    {
        return false;
    }
    keyLoadAttempted_ = true;

    std::array<std::uint8_t, SimpleModulus::KeyFileSize> enc1{};
    std::array<std::uint8_t, SimpleModulus::KeyFileSize> dec2{};
    std::string enc1Path;
    std::string dec2Path;

    if (!ReadKeyFile(kEnc1PathEnv, "Data/Enc1.dat", enc1, enc1Path))
    {
        mu::log::Get("network")->error(
            "S52: failed to read Enc1 key file '{}' (expected {} bytes)",
            enc1Path, SimpleModulus::KeyFileSize);
        return false;
    }

    if (!ReadKeyFile(kDec2PathEnv, "Data/Dec2.dat", dec2, dec2Path))
    {
        mu::log::Get("network")->error(
            "S52: failed to read Dec2 key file '{}' (expected {} bytes)",
            dec2Path, SimpleModulus::KeyFileSize);
        return false;
    }

    if (!crypto_.LoadClientToServerKey(enc1.data(), enc1.size()))
    {
        mu::log::Get("network")->error(
            "S52: invalid Enc1 key file '{}'", enc1Path);
        return false;
    }

    if (!crypto_.LoadServerToClientKey(dec2.data(), dec2.size()))
    {
        mu::log::Get("network")->error(
            "S52: invalid Dec2 key file '{}'", dec2Path);
        return false;
    }

    keysLoaded_ = true;
    mu::log::Get("network")->info(
        "S52: loaded direct protocol keys Enc1='{}' Dec2='{}'",
        enc1Path, dec2Path);
    return true;
}

bool DirectSession::SendPacket(Connection* connection,
                               const std::vector<std::uint8_t>& packet)
{
    if (connection == nullptr || !connection->IsConnected() || packet.empty())
    {
        return false;
    }

    connection->Send(
        reinterpret_cast<const BYTE*>(packet.data()),
        static_cast<int32_t>(packet.size()));
    return true;
}

bool DirectSession::SendXorPacket(
    Connection* connection,
    std::vector<std::uint8_t> packet)
{
    if (!DirectProtocolEnabled() || !gameServer_ || packet.empty())
    {
        return false;
    }

    if (!EncodePacketXor(packet.data(), packet.size()))
    {
        return false;
    }

    return SendPacket(connection, packet);
}

bool DirectSession::SendEncryptedPacket(
    Connection* connection,
    const std::vector<std::uint8_t>& canonicalPacket)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    if (!crypto_.Encode(
            canonicalPacket.data(),
            canonicalPacket.size(),
            wire))
    {
        mu::log::Get("network")->error(
            "S52: failed to encrypt outgoing packet head=0x{:02X}",
            canonicalPacket.size() > 2 ? canonicalPacket[2] : 0);
        return false;
    }

    return SendPacket(connection, wire);
}

bool DirectSession::SendServerList(Connection* connection)
{
    if (!DirectProtocolEnabled())
    {
        return false;
    }

    return SendPacket(connection, BuildServerListRequest());
}

bool DirectSession::SendServerAddress(Connection* connection,
                                      std::uint16_t serverId)
{
    if (!DirectProtocolEnabled())
    {
        return false;
    }

    return SendPacket(connection, BuildServerAddressRequest(serverId));
}

bool DirectSession::SendCharacterList(Connection* connection,
                                      std::uint8_t language)
{
    if (!DirectProtocolEnabled() || !gameServer_)
    {
        return false;
    }

    return SendPacket(connection, BuildCharacterListRequest(language));
}

bool DirectSession::SendCreateCharacter(Connection* connection,
                                        const wchar_t* name,
                                        std::uint8_t classId,
                                        std::uint8_t skin)
{
    if (!DirectProtocolEnabled() || !gameServer_ || name == nullptr)
    {
        return false;
    }

    std::array<char, CharacterNameSize + 1> nameUtf8{};
    CMultiLanguage::ConvertToUtf8(
        nameUtf8.data(), name, static_cast<int>(nameUtf8.size()));

    return SendPacket(
        connection,
        BuildCreateCharacterRequest(nameUtf8.data(), classId, skin));
}

bool DirectSession::SendDeleteCharacter(Connection* connection,
                                        const wchar_t* name,
                                        const wchar_t* personalCode)
{
    if (!DirectProtocolEnabled() || !gameServer_
        || name == nullptr || personalCode == nullptr)
    {
        return false;
    }

    std::array<char, CharacterNameSize + 1> nameUtf8{};
    std::array<char, PersonalCodeSize + 1> codeUtf8{};

    CMultiLanguage::ConvertToUtf8(
        nameUtf8.data(), name, static_cast<int>(nameUtf8.size()));
    CMultiLanguage::ConvertToUtf8(
        codeUtf8.data(), personalCode, static_cast<int>(codeUtf8.size()));

    return SendPacket(
        connection,
        BuildDeleteCharacterRequest(nameUtf8.data(), codeUtf8.data()));
}

bool DirectSession::SendSelectCharacter(Connection* connection,
                                        const wchar_t* name)
{
    if (!DirectProtocolEnabled() || !gameServer_ || name == nullptr)
    {
        return false;
    }

    std::array<char, CharacterNameSize + 1> nameUtf8{};
    CMultiLanguage::ConvertToUtf8(
        nameUtf8.data(), name, static_cast<int>(nameUtf8.size()));

    return SendPacket(connection, BuildSelectCharacterRequest(nameUtf8.data()));
}

bool DirectSession::SendFinishLoading(Connection* connection)
{
    if (!DirectProtocolEnabled() || !gameServer_)
    {
        return false;
    }

    return SendPacket(connection, BuildFinishLoadingRequest());
}

bool DirectSession::SendMapServerMoveAuth(
    Connection* connection,
    const wchar_t* account,
    const wchar_t* character,
    std::uint32_t authCode1,
    std::uint32_t authCode2,
    std::uint32_t authCode3,
    std::uint32_t authCode4,
    std::uint32_t tickCount,
    const std::uint8_t* version,
    const std::uint8_t* serial)
{
    if (!DirectProtocolEnabled() || !gameServer_
        || connection == nullptr || account == nullptr || character == nullptr
        || version == nullptr || serial == nullptr
        || !EnsureKeysLoaded())
    {
        return false;
    }

    std::array<char, 13> accountUtf8{};
    std::array<char, 13> characterUtf8{};
    CMultiLanguage::ConvertToUtf8(
        accountUtf8.data(), account, static_cast<int>(accountUtf8.size()));
    CMultiLanguage::ConvertToUtf8(
        characterUtf8.data(), character, static_cast<int>(characterUtf8.size()));

    (void)version;
    (void)serial;

    std::array<std::uint8_t, ProtocolVersionSize> protocolVersion{};
    std::array<std::uint8_t, ProtocolSerialSize> protocolSerial{};
    if (!ResolveProtocolIdentity(protocolVersion, protocolSerial))
    {
        mu::log::Get("network")->error(
            "S52: invalid MU_S52_VERSION or MU_S52_SERIAL configuration");
        return false;
    }

    std::vector<std::uint8_t> wire;
    if (!BuildMapServerMoveAuthRequest(
            accountUtf8.data(),
            characterUtf8.data(),
            authCode1,
            authCode2,
            authCode3,
            authCode4,
            tickCount,
            protocolVersion,
            protocolSerial,
            crypto_,
            wire))
    {
        mu::log::Get("network")->error(
            "S52: failed to build encrypted B1:01 map-server auth packet");
        return false;
    }

    mu::log::Get("network")->info(
        "S52: sending encrypted B1:01 map-server auth ({} bytes)", wire.size());
    return SendPacket(connection, wire);
}

bool DirectSession::SendWalk(Connection* connection,
                             std::uint8_t sourceX,
                             std::uint8_t sourceY,
                             std::uint8_t stepCount,
                             std::uint8_t targetRotation,
                             const std::uint8_t* directions,
                             std::size_t directionsSize)
{
    if (stepCount > 0x0F || (directionsSize > 0 && directions == nullptr))
    {
        return false;
    }

    std::vector<std::uint8_t> packet{
        0xC1,
        0,
        0xD4,
        sourceX,
        sourceY,
        static_cast<std::uint8_t>(
            ((targetRotation & 0x0F) << 4) | (stepCount & 0x0F))
    };

    if (directionsSize > 0)
    {
        packet.insert(packet.end(), directions, directions + directionsSize);
    }

    if (packet.size() >= 256)
    {
        return false;
    }

    packet[1] = static_cast<std::uint8_t>(packet.size());
    return SendXorPacket(connection, std::move(packet));
}

bool DirectSession::SendInstantMove(Connection* connection,
                                    std::uint8_t targetX,
                                    std::uint8_t targetY)
{
    return SendXorPacket(
        connection,
        {0xC1, 0x05, 0x15, targetX, targetY});
}

bool DirectSession::SendEnterGate(Connection* connection,
                                  std::uint16_t gateNumber,
                                  std::uint8_t targetX,
                                  std::uint8_t targetY)
{
    // Louis Main SendRequestMagicTeleport():
    // C1:1C + alignment byte + WORD gate + X + Y, Send(TRUE).
    return SendEncryptedPacket(
        connection,
        {
            0xC1, 0x08, 0x1C, 0x00,
            static_cast<std::uint8_t>(gateNumber & 0xFFu),
            static_cast<std::uint8_t>((gateNumber >> 8u) & 0xFFu),
            targetX,
            targetY
        });
}

bool DirectSession::SendWarpCommand(Connection* connection,
                                    std::uint32_t commandKey,
                                    std::uint16_t mapIndex)
{
    // Louis Main SendRequestMoveMap(): C1:8E:02 + DWORD key + WORD map.
    return SendXorPacket(
        connection,
        {
            0xC1, 0x0A, 0x8E, 0x02,
            static_cast<std::uint8_t>(commandKey & 0xFFu),
            static_cast<std::uint8_t>((commandKey >> 8u) & 0xFFu),
            static_cast<std::uint8_t>((commandKey >> 16u) & 0xFFu),
            static_cast<std::uint8_t>((commandKey >> 24u) & 0xFFu),
            static_cast<std::uint8_t>(mapIndex & 0xFFu),
            static_cast<std::uint8_t>((mapIndex >> 8u) & 0xFFu)
        });
}

bool DirectSession::SendAnimation(Connection* connection,
                                  std::uint8_t rotation,
                                  std::uint8_t animationNumber)
{
    return SendXorPacket(
        connection,
        {0xC1, 0x05, 0x18, rotation, animationNumber});
}

bool DirectSession::SendHit(Connection* connection,
                            std::uint16_t targetId,
                            std::uint8_t attackAnimation,
                            std::uint8_t lookingDirection)
{
    return SendXorPacket(
        connection,
        {
            0xC1,
            0x07,
            0x11,
            static_cast<std::uint8_t>((targetId >> 8) & 0xFF),
            static_cast<std::uint8_t>(targetId & 0xFF),
            attackAnimation,
            lookingDirection
        });
}

bool DirectSession::SendTargetedSkill(Connection* connection,
                                      std::uint16_t skillId,
                                      std::uint16_t targetId)
{
    return SendEncryptedPacket(
        connection,
        {
            0xC1,
            0x07,
            0x19,
            static_cast<std::uint8_t>((skillId >> 8) & 0xFF),
            static_cast<std::uint8_t>(skillId & 0xFF),
            static_cast<std::uint8_t>((targetId >> 8) & 0xFF),
            static_cast<std::uint8_t>(targetId & 0xFF)
        });
}

bool DirectSession::SendPickupItem(Connection* connection,
                                   std::uint16_t itemId)
{
    return SendEncryptedPacket(
        connection,
        {
            0xC1,
            0x05,
            0x22,
            static_cast<std::uint8_t>((itemId >> 8) & 0xFF),
            static_cast<std::uint8_t>(itemId & 0xFF)
        });
}

bool DirectSession::SendDropItem(Connection* connection,
                                 std::uint8_t targetX,
                                 std::uint8_t targetY,
                                 std::uint8_t itemSlot)
{
    return SendEncryptedPacket(
        connection,
        {0xC1, 0x06, 0x23, targetX, targetY, itemSlot});
}

bool DirectSession::SendInventoryRequest(Connection* connection)
{
    // Louis Main 5.2 SendRequestInventory(): C1:F3:10, Send(TRUE).
    return SendEncryptedPacket(connection, {0xC1, 0x04, 0xF3, 0x10});
}

bool DirectSession::SendCrywolfInfoRequest(Connection* connection)
{
    // Louis Main 5.2 SendRequestCrywolfInfo(): C1:BD:00, Send().
    // CStreamPacketEngine XORs payload bytes even when SimpleModulus is off.
    return SendXorPacket(connection, {0xC1, 0x04, 0xBD, 0x00});
}

bool DirectSession::SendCrywolfChaosRateBenefitRequest(Connection* connection)
{
    // Louis Main 5.2 SendRequestCrywolfBenefitPlusChaosRate():
    // C1:BD:09, Send().
    return SendXorPacket(connection, {0xC1, 0x04, 0xBD, 0x09});
}

bool DirectSession::SendChecksumResponse(
    Connection* connection, std::uint32_t checksum)
{
    // Louis Main 5.2 SendCheckSum():
    // C1:03 + one reserved byte + DWORD checksum, Send(TRUE).
    return SendEncryptedPacket(
        connection,
        {
            0xC1, 0x08, 0x03, 0x00,
            static_cast<std::uint8_t>(checksum & 0xFFu),
            static_cast<std::uint8_t>((checksum >> 8u) & 0xFFu),
            static_cast<std::uint8_t>((checksum >> 16u) & 0xFFu),
            static_cast<std::uint8_t>((checksum >> 24u) & 0xFFu)
        });
}

bool DirectSession::SendCastleSiegeStatusRequest(Connection* connection)
{
    // Louis Main 5.2 SendRequestBCStatus(): C1:B2:00, Send().
    return SendXorPacket(connection, {0xC1, 0x04, 0xB2, 0x00});
}

bool DirectSession::SendCastleSiegeRegistrationStateRequest(
    Connection* connection)
{
    // Louis Main 5.2 SendRequestBCRegInfo(): C1:B2:03, Send().
    return SendXorPacket(connection, {0xC1, 0x04, 0xB2, 0x03});
}

bool DirectSession::SendCastleSiegeRegisteredGuildsListRequest(
    Connection* connection)
{
    // Louis Main 5.2 SendRequestBCDeclareGuildList(): C1:B4, Send().
    return SendXorPacket(connection, {0xC1, 0x03, 0xB4});
}

bool DirectSession::SendItemMove(
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
    std::uint8_t targetIndex)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    return BuildItemMoveRequest(
               sourceStorage,
               sourceIndex,
               itemTypeLow,
               itemLevel,
               durability,
               option1,
               excellentOption,
               splitType,
               spareBits,
               socketOptions,
               targetStorage,
               targetIndex,
               crypto_,
               wire)
        && SendPacket(connection, wire);
}

bool DirectSession::SendAreaSkill(Connection* connection,
                                  std::uint16_t skillId,
                                  std::uint8_t targetX,
                                  std::uint8_t targetY,
                                  std::uint8_t angle,
                                  std::uint8_t destination,
                                  std::uint8_t targetPosition,
                                  std::uint16_t targetId,
                                  std::uint8_t skillSerial)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    if (!BuildAreaSkillRequest(
            skillId,
            targetX,
            targetY,
            angle,
            destination,
            targetPosition,
            targetId,
            skillSerial,
            crypto_,
            wire))
    {
        return false;
    }

    return SendPacket(connection, wire);
}

bool DirectSession::SendTalkNpc(Connection* connection, std::uint16_t npcId)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    return BuildTalkNpcRequest(npcId, crypto_, wire)
        && SendPacket(connection, wire);
}

bool DirectSession::SendCloseNpc(Connection* connection)
{
    if (!DirectProtocolEnabled() || !gameServer_)
    {
        return false;
    }

    return SendPacket(connection, BuildCloseNpcRequest());
}

bool DirectSession::SendConsumeItem(Connection* connection,
                                    std::uint8_t itemSlot,
                                    std::uint8_t targetSlot,
                                    std::uint8_t fruitUsage)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    return BuildConsumeItemRequest(
               itemSlot, targetSlot, fruitUsage, crypto_, wire)
        && SendPacket(connection, wire);
}

bool DirectSession::SendBuyItem(Connection* connection, std::uint8_t itemSlot)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    return BuildBuyItemRequest(itemSlot, crypto_, wire)
        && SendPacket(connection, wire);
}

bool DirectSession::SendSellItem(Connection* connection, std::uint8_t itemSlot)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    return BuildSellItemRequest(itemSlot, crypto_, wire)
        && SendPacket(connection, wire);
}

bool DirectSession::SendRepairItem(Connection* connection,
                                   std::uint8_t itemSlot,
                                   std::uint8_t addGold)
{
    if (!DirectProtocolEnabled() || !gameServer_ || !EnsureKeysLoaded())
    {
        return false;
    }

    std::vector<std::uint8_t> wire;
    return BuildRepairItemRequest(itemSlot, addGold, crypto_, wire)
        && SendPacket(connection, wire);
}

bool DirectSession::SendPublicChat(Connection* connection,
                                   const wchar_t* character,
                                   const wchar_t* text)
{
    if (!DirectProtocolEnabled() || !gameServer_
        || character == nullptr || text == nullptr)
    {
        return false;
    }

    std::array<char, CharacterNameSize + 1> characterUtf8{};
    std::array<char, 91> textUtf8{};
    CMultiLanguage::ConvertToUtf8(
        characterUtf8.data(), character, static_cast<int>(characterUtf8.size()));
    CMultiLanguage::ConvertToUtf8(
        textUtf8.data(), text, static_cast<int>(textUtf8.size()));

    return SendPacket(
        connection,
        BuildPublicChatRequest(characterUtf8.data(), textUtf8.data()));
}

bool DirectSession::SendWhisper(Connection* connection,
                                const wchar_t* target,
                                const wchar_t* text)
{
    if (!DirectProtocolEnabled() || !gameServer_
        || connection == nullptr || target == nullptr || text == nullptr)
    {
        return false;
    }

    std::array<char, CharacterNameSize + 1> targetUtf8{};
    std::array<char, 91> textUtf8{};
    CMultiLanguage::ConvertToUtf8(
        targetUtf8.data(), target, static_cast<int>(targetUtf8.size()));
    CMultiLanguage::ConvertToUtf8(
        textUtf8.data(), text, static_cast<int>(textUtf8.size()));

    return SendPacket(
        connection,
        BuildWhisperRequest(targetUtf8.data(), textUtf8.data()));
}

bool DirectSession::SendTradeRequest(Connection* connection,
                                     std::uint16_t targetId)
{
    return SendEncryptedPacket(
        connection,
        {
            0xC1, 0x05, 0x36,
            static_cast<std::uint8_t>((targetId >> 8u) & 0xFFu),
            static_cast<std::uint8_t>(targetId & 0xFFu)
        });
}

bool DirectSession::SendTradeResponse(Connection* connection,
                                      std::uint8_t accepted)
{
    return SendXorPacket(connection, {0xC1, 0x04, 0x37, accepted});
}

bool DirectSession::SendTradeMoney(Connection* connection,
                                   std::uint32_t amount)
{
    std::vector<std::uint8_t> packet{
        0xC1, 0x08, 0x3A, 0x00,
        static_cast<std::uint8_t>(amount & 0xFFu),
        static_cast<std::uint8_t>((amount >> 8u) & 0xFFu),
        static_cast<std::uint8_t>((amount >> 16u) & 0xFFu),
        static_cast<std::uint8_t>((amount >> 24u) & 0xFFu)
    };
    return SendXorPacket(connection, std::move(packet));
}

bool DirectSession::SendTradeResult(Connection* connection,
                                    std::uint8_t accepted)
{
    return SendEncryptedPacket(
        connection, {0xC1, 0x04, 0x3C, accepted});
}

bool DirectSession::SendTradeExit(Connection* connection)
{
    return SendEncryptedPacket(connection, {0xC1, 0x03, 0x3D});
}

bool DirectSession::SendPartyInvite(Connection* connection,
                                    std::uint16_t targetId)
{
    return SendEncryptedPacket(
        connection,
        {
            0xC1, 0x05, 0x40,
            static_cast<std::uint8_t>((targetId >> 8u) & 0xFFu),
            static_cast<std::uint8_t>(targetId & 0xFFu)
        });
}

bool DirectSession::SendPartyResponse(Connection* connection,
                                      std::uint8_t accepted,
                                      std::uint16_t requesterId)
{
    return SendEncryptedPacket(
        connection,
        {
            0xC1, 0x06, 0x41, accepted,
            static_cast<std::uint8_t>((requesterId >> 8u) & 0xFFu),
            static_cast<std::uint8_t>(requesterId & 0xFFu)
        });
}

bool DirectSession::SendPartyList(Connection* connection)
{
    return SendXorPacket(connection, {0xC1, 0x03, 0x42});
}

bool DirectSession::SendPartyLeave(Connection* connection,
                                   std::uint8_t partyIndex)
{
    return SendXorPacket(
        connection, {0xC1, 0x04, 0x43, partyIndex});
}

bool DirectSession::SendPing(Connection* connection)
{
    // Louis Main 5.2: C1:71 with no payload, spe.Send().
    return SendXorPacket(connection, {0xC1, 0x03, 0x71});
}

bool DirectSession::SendQuestHistory(Connection* connection)
{
    // Louis Main SendRequestQuestHistory(): C1:A0, spe.Send(TRUE).
    return SendEncryptedPacket(connection, BuildQuestHistoryRequest());
}

bool DirectSession::SendQuestState(Connection* connection,
                                   std::uint8_t questIndex,
                                   std::uint8_t questState)
{
    // Louis Main SendRequestQuestState(): C1:A2 + index/state,
    // spe.Send(TRUE).
    return SendEncryptedPacket(
        connection, BuildQuestStateRequest(questIndex, questState));
}

bool DirectSession::SendVaultMoveMoney(Connection* connection,
                                       std::uint8_t direction,
                                       std::uint32_t amount)
{
    // Louis Main SendRequestStorageGold(): C1:81 + BYTE flag + DWORD gold.
    return SendXorPacket(
        connection,
        {
            0xC1, 0x08, 0x81, direction,
            static_cast<std::uint8_t>(amount & 0xFFu),
            static_cast<std::uint8_t>((amount >> 8u) & 0xFFu),
            static_cast<std::uint8_t>((amount >> 16u) & 0xFFu),
            static_cast<std::uint8_t>((amount >> 24u) & 0xFFu)
        });
}

bool DirectSession::SendVaultClosed(Connection* connection)
{
    // Louis Main SendRequestStorageExit(): C1:82, spe.Send().
    return SendXorPacket(connection, {0xC1, 0x03, 0x82});
}

bool DirectSession::SendGuildJoinRequest(Connection* connection,
                                         std::uint16_t targetId)
{
    return SendXorPacket(
        connection,
        {0xC1, 0x05, 0x50,
         static_cast<std::uint8_t>((targetId >> 8u) & 0xFFu),
         static_cast<std::uint8_t>(targetId & 0xFFu)});
}

bool DirectSession::SendGuildJoinResponse(Connection* connection,
                                          std::uint8_t accepted,
                                          std::uint16_t requesterId)
{
    return SendXorPacket(
        connection,
        {0xC1, 0x06, 0x51, accepted,
         static_cast<std::uint8_t>((requesterId >> 8u) & 0xFFu),
         static_cast<std::uint8_t>(requesterId & 0xFFu)});
}

bool DirectSession::SendGuildListRequest(Connection* connection)
{
    return SendXorPacket(connection, {0xC1, 0x03, 0x52});
}

bool DirectSession::SendGuildLeave(Connection* connection,
                                   const wchar_t* character,
                                   const wchar_t* personalCode)
{
    if (character == nullptr || personalCode == nullptr)
    {
        return false;
    }

    std::array<char, CharacterNameSize + 1> characterUtf8{};
    std::array<char, PersonalCodeSize + 1> codeUtf8{};
    CMultiLanguage::ConvertToUtf8(
        characterUtf8.data(), character, static_cast<int>(characterUtf8.size()));
    CMultiLanguage::ConvertToUtf8(
        codeUtf8.data(), personalCode, static_cast<int>(codeUtf8.size()));

    std::vector<std::uint8_t> packet{0xC1, 0x21, 0x53};
    packet.insert(packet.end(),
                  reinterpret_cast<const std::uint8_t*>(characterUtf8.data()),
                  reinterpret_cast<const std::uint8_t*>(characterUtf8.data()) + CharacterNameSize);
    packet.insert(packet.end(),
                  reinterpret_cast<const std::uint8_t*>(codeUtf8.data()),
                  reinterpret_cast<const std::uint8_t*>(codeUtf8.data()) + PersonalCodeSize);
    return SendXorPacket(connection, std::move(packet));
}

bool DirectSession::SendGuildMasterAnswer(Connection* connection,
                                          std::uint8_t value)
{
    return SendXorPacket(connection, {0xC1, 0x04, 0x54, value});
}

bool DirectSession::SendGuildCreate(Connection* connection,
                                    std::uint8_t guildType,
                                    const wchar_t* guildName,
                                    const std::uint8_t* guildMark,
                                    std::size_t guildMarkSize)
{
    if (guildName == nullptr || guildMark == nullptr || guildMarkSize < 32u)
    {
        return false;
    }

    std::array<char, 9> guildNameUtf8{};
    CMultiLanguage::ConvertToUtf8(
        guildNameUtf8.data(), guildName, static_cast<int>(guildNameUtf8.size()));

    std::vector<std::uint8_t> packet{0xC1, 0x2C, 0x55, guildType};
    packet.insert(packet.end(),
                  reinterpret_cast<const std::uint8_t*>(guildNameUtf8.data()),
                  reinterpret_cast<const std::uint8_t*>(guildNameUtf8.data()) + 8);
    packet.insert(packet.end(), guildMark, guildMark + 32);
    return SendXorPacket(connection, std::move(packet));
}

bool DirectSession::SendGuildTypeChange(Connection* connection,
                                        std::uint8_t guildType)
{
    return SendXorPacket(connection, {0xC1, 0x04, 0xE2, guildType});
}

bool DirectSession::SendGuildCreationCancel(Connection* connection)
{
    return SendXorPacket(connection, {0xC1, 0x03, 0x57});
}

bool DirectSession::SendGuildWarDeclare(Connection* connection,
                                        const wchar_t* guildName)
{
    if (guildName == nullptr)
    {
        return false;
    }

    std::array<char, 9> guildNameUtf8{};
    CMultiLanguage::ConvertToUtf8(
        guildNameUtf8.data(), guildName, static_cast<int>(guildNameUtf8.size()));

    std::vector<std::uint8_t> packet{0xC1, 0x0B, 0x60};
    packet.insert(packet.end(),
                  reinterpret_cast<const std::uint8_t*>(guildNameUtf8.data()),
                  reinterpret_cast<const std::uint8_t*>(guildNameUtf8.data()) + 8);
    return SendXorPacket(connection, std::move(packet));
}

bool DirectSession::SendGuildWarResponse(Connection* connection,
                                         std::uint8_t accepted)
{
    return SendXorPacket(connection, {0xC1, 0x04, 0x61, accepted});
}

bool DirectSession::SendGuildInfoRequest(Connection* connection,
                                         std::uint32_t guildKey)
{
    return SendXorPacket(
        connection,
        {0xC1, 0x08, 0x66, 0x00,
         static_cast<std::uint8_t>(guildKey & 0xFFu),
         static_cast<std::uint8_t>((guildKey >> 8u) & 0xFFu),
         static_cast<std::uint8_t>((guildKey >> 16u) & 0xFFu),
         static_cast<std::uint8_t>((guildKey >> 24u) & 0xFFu)});
}

bool DirectSession::SendGuildRoleAssign(Connection* connection,
                                        std::uint8_t type,
                                        std::uint8_t guildStatus,
                                        const wchar_t* playerName)
{
    if (playerName == nullptr)
    {
        return false;
    }

    std::array<char, CharacterNameSize + 1> nameUtf8{};
    CMultiLanguage::ConvertToUtf8(
        nameUtf8.data(), playerName, static_cast<int>(nameUtf8.size()));

    std::vector<std::uint8_t> packet{0xC1, 0x0F, 0xE1, type, guildStatus};
    packet.insert(packet.end(),
                  reinterpret_cast<const std::uint8_t*>(nameUtf8.data()),
                  reinterpret_cast<const std::uint8_t*>(nameUtf8.data()) + CharacterNameSize);
    return SendXorPacket(connection, std::move(packet));
}

bool DirectSession::SendGuildRelationshipRequest(Connection* connection,
                                                 std::uint8_t relationshipType,
                                                 std::uint8_t requestType,
                                                 std::uint16_t targetPlayerId)
{
    return SendXorPacket(
        connection,
        {0xC1, 0x07, 0xE5, relationshipType, requestType,
         static_cast<std::uint8_t>((targetPlayerId >> 8u) & 0xFFu),
         static_cast<std::uint8_t>(targetPlayerId & 0xFFu)});
}

bool DirectSession::SendGuildRelationshipResponse(Connection* connection,
                                                  std::uint8_t relationshipType,
                                                  std::uint8_t requestType,
                                                  std::uint8_t result,
                                                  std::uint16_t targetPlayerId)
{
    return SendXorPacket(
        connection,
        {0xC1, 0x08, 0xE6, relationshipType, requestType, result,
         static_cast<std::uint8_t>((targetPlayerId >> 8u) & 0xFFu),
         static_cast<std::uint8_t>(targetPlayerId & 0xFFu)});
}

bool DirectSession::SendAllianceListRequest(Connection* connection)
{
    return SendXorPacket(connection, {0xC1, 0x03, 0xE9});
}

bool DirectSession::SendAllianceGuildBan(Connection* connection,
                                         const wchar_t* guildName)
{
    if (guildName == nullptr)
    {
        return false;
    }

    std::array<char, 9> guildNameUtf8{};
    CMultiLanguage::ConvertToUtf8(
        guildNameUtf8.data(), guildName, static_cast<int>(guildNameUtf8.size()));

    std::vector<std::uint8_t> packet{0xC1, 0x0C, 0xEB, 0x01};
    packet.insert(packet.end(),
                  reinterpret_cast<const std::uint8_t*>(guildNameUtf8.data()),
                  reinterpret_cast<const std::uint8_t*>(guildNameUtf8.data()) + 8);
    return SendXorPacket(connection, std::move(packet));
}

bool DirectSession::SendLogin(Connection* connection,
                              const wchar_t* account,
                              const wchar_t* password,
                              const std::uint8_t* version,
                              const std::uint8_t* serial)
{
    if (!DirectProtocolEnabled() || !gameServer_
        || connection == nullptr || account == nullptr || password == nullptr
        || version == nullptr || serial == nullptr
        || !EnsureKeysLoaded())
    {
        return false;
    }

    std::array<char, AccountSize + 1> accountUtf8{};
    std::array<char, PasswordSize + 1> passwordUtf8{};

    CMultiLanguage::ConvertToUtf8(
        accountUtf8.data(), account, static_cast<int>(accountUtf8.size()));
    CMultiLanguage::ConvertToUtf8(
        passwordUtf8.data(), password, static_cast<int>(passwordUtf8.size()));

    // Direct S5.2 has its own protocol identity. Do not reuse the OpenMU
    // globals (currently 2.04.04 / k1Pk...), because Louis EX502 defaults to
    // 1.04.05 / TbYehR2hFUPBKgZj. Environment variables can override both.
    (void)version;
    (void)serial;

    std::array<std::uint8_t, ProtocolVersionSize> protocolVersion{};
    std::array<std::uint8_t, ProtocolSerialSize> protocolSerial{};
    if (!ResolveProtocolIdentity(protocolVersion, protocolSerial))
    {
        mu::log::Get("network")->error(
            "S52: invalid MU_S52_VERSION or MU_S52_SERIAL configuration");
        return false;
    }

    std::vector<std::uint8_t> wire;
    if (!BuildLoginRequest(
            accountUtf8.data(),
            passwordUtf8.data(),
            static_cast<std::uint32_t>(GetTickCount()),
            protocolVersion,
            protocolSerial,
            crypto_,
            wire))
    {
        mu::log::Get("network")->error("S52: failed to build F1:01 login packet");
        return false;
    }

    mu::log::Get("network")->info(
        "S52: sending encrypted F1:01 login packet ({} bytes)", wire.size());
    return SendPacket(connection, wire);
}

bool DirectSession::ProcessIncoming(
    const std::uint8_t* data,
    std::size_t size,
    std::vector<std::vector<std::uint8_t>>& packets)
{
    packets.clear();
    if (!DirectProtocolEnabled())
    {
        return false;
    }

    std::vector<std::vector<std::uint8_t>> framed;
    if (!stream_.Feed(data, size, framed))
    {
        mu::log::Get("network")->error(
            "S52: invalid C1/C2/C3/C4 stream framing; dropping buffered bytes");
        return false;
    }

    for (auto& packet : framed)
    {
        if (!gameServer_ || packet.empty()
            || (packet[0] != 0xC3 && packet[0] != 0xC4))
        {
            packets.push_back(std::move(packet));
            continue;
        }

        if (!EnsureKeysLoaded())
        {
            return false;
        }

        const auto wireType = packet[0];
        std::vector<std::uint8_t> decoded;
        if (!crypto_.Decode(packet.data(), packet.size(), decoded))
        {
            mu::log::Get("network")->error(
                "S52: failed to decrypt incoming {} packet ({} bytes)",
                wireType == 0xC3 ? "C3" : "C4", packet.size());
            return false;
        }

        // Keep the legacy WSclient bEncrypted behavior while feeding it the
        // already-decoded payload. C3 has the same header width as C1 and C4
        // the same header width as C2, so only the type byte needs restoring.
        decoded[0] = wireType;
        packets.push_back(std::move(decoded));
    }

    return true;
}

} // namespace mu::net::s52
