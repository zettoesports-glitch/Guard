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

bool IsEnabledValue(const char* value) noexcept
{
    return value != nullptr
        && (std::strcmp(value, "1") == 0
            || std::strcmp(value, "true") == 0
            || std::strcmp(value, "TRUE") == 0);
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

    std::array<std::uint8_t, ProtocolVersionSize> protocolVersion{};
    std::array<std::uint8_t, ProtocolSerialSize> protocolSerial{};

    std::copy_n(version, protocolVersion.size(), protocolVersion.begin());
    std::copy_n(serial, protocolSerial.size(), protocolSerial.begin());

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
