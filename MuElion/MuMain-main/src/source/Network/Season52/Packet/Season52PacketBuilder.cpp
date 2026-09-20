#include "Season52PacketBuilder.hpp"
#include "../Crypto/Season52Crypto.hpp"
#include "../Crypto/Season52Xor.hpp"

#include <algorithm>
#include <array>

namespace mu::net::s52 {
namespace {

void WriteU32LE(std::vector<std::uint8_t>& packet, std::uint32_t value) {
    packet.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    packet.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xFFu));
    packet.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xFFu));
    packet.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xFFu));
}

template <std::size_t N>
std::array<std::uint8_t, N> FixedField(std::string_view value) {
    std::array<std::uint8_t, N> field{};
    const auto count = std::min(value.size(), field.size());
    std::copy_n(reinterpret_cast<const std::uint8_t*>(value.data()), count, field.begin());
    return field;
}

} // namespace

std::vector<std::uint8_t> BuildServerListRequest() {
    return {0xC1, 0x04, 0xF4, 0x06};
}

std::vector<std::uint8_t> BuildServerAddressRequest(std::uint16_t serverId) {
    return {
        0xC1, 0x06, 0xF4, 0x03,
        static_cast<std::uint8_t>(serverId & 0xFFu),
        static_cast<std::uint8_t>((serverId >> 8u) & 0xFFu)
    };
}

std::vector<std::uint8_t> BuildCharacterListRequest(std::uint8_t language) {
    std::vector<std::uint8_t> packet{0xC1, 0x05, 0xF3, 0x00, language};
    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

std::vector<std::uint8_t> BuildCreateCharacterRequest(std::string_view name,
                                                       std::uint8_t classId,
                                                       std::uint8_t skin) {
    const auto nameField = FixedField<CharacterNameSize>(name);

    std::vector<std::uint8_t> packet{
        0xC1,
        static_cast<std::uint8_t>(4u + CharacterNameSize + 1u),
        0xF3,
        0x01
    };
    packet.insert(packet.end(), nameField.begin(), nameField.end());
    packet.push_back(static_cast<std::uint8_t>((classId << 4u) + skin));

    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

std::vector<std::uint8_t> BuildDeleteCharacterRequest(std::string_view name,
                                                       std::string_view personalCode) {
    const auto nameField = FixedField<CharacterNameSize>(name);
    const auto codeField = FixedField<PersonalCodeSize>(personalCode);

    std::vector<std::uint8_t> packet{
        0xC1,
        static_cast<std::uint8_t>(4u + CharacterNameSize + PersonalCodeSize),
        0xF3,
        0x02
    };
    packet.insert(packet.end(), nameField.begin(), nameField.end());
    packet.insert(packet.end(), codeField.begin(), codeField.end());

    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

std::vector<std::uint8_t> BuildSelectCharacterRequest(std::string_view name) {
    const auto nameField = FixedField<CharacterNameSize>(name);

    std::vector<std::uint8_t> packet{
        0xC1,
        static_cast<std::uint8_t>(4u + CharacterNameSize),
        0xF3,
        0x03
    };
    packet.insert(packet.end(), nameField.begin(), nameField.end());

    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

std::vector<std::uint8_t> BuildFinishLoadingRequest() {
    std::vector<std::uint8_t> packet{0xC1, 0x04, 0xF3, 0x12};
    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

std::vector<std::uint8_t> BuildCloseNpcRequest() {
    std::vector<std::uint8_t> packet{0xC1, 0x03, 0x31};
    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

std::vector<std::uint8_t> BuildPublicChatRequest(std::string_view character,
                                                 std::string_view text) {
    constexpr std::size_t ChatSize = 90;
    const auto characterField = FixedField<CharacterNameSize>(character);
    const auto textBytes = std::min(text.size() + 1u, ChatSize);
    const auto textChars = std::min(text.size(), textBytes);

    std::vector<std::uint8_t> packet;
    packet.reserve(3u + CharacterNameSize + textBytes);
    packet.push_back(0xC1);
    packet.push_back(0);
    packet.push_back(0x00);
    packet.insert(packet.end(), characterField.begin(), characterField.end());
    if (textChars > 0) {
        packet.insert(
            packet.end(),
            reinterpret_cast<const std::uint8_t*>(text.data()),
            reinterpret_cast<const std::uint8_t*>(text.data()) + textChars);
    }

    if (textBytes > textChars) {
        packet.push_back(0);
    }

    packet[1] = static_cast<std::uint8_t>(packet.size());
    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

std::vector<std::uint8_t> BuildWhisperRequest(std::string_view target,
                                              std::string_view text) {
    constexpr std::size_t ChatSize = 90;
    const auto targetField = FixedField<CharacterNameSize>(target);
    const auto textBytes = std::min(text.size() + 1u, ChatSize);
    const auto textChars = std::min(text.size(), textBytes);

    std::vector<std::uint8_t> packet;
    packet.reserve(3u + CharacterNameSize + textBytes);
    packet.push_back(0xC1);
    packet.push_back(0);
    packet.push_back(0x02);
    packet.insert(packet.end(), targetField.begin(), targetField.end());

    if (textChars > 0) {
        packet.insert(
            packet.end(),
            reinterpret_cast<const std::uint8_t*>(text.data()),
            reinterpret_cast<const std::uint8_t*>(text.data()) + textChars);
    }

    if (textBytes > textChars) {
        packet.push_back(0);
    }

    packet[1] = static_cast<std::uint8_t>(packet.size());
    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

bool BuildAreaSkillRequest(std::uint16_t skillId,
                           std::uint8_t targetX,
                           std::uint8_t targetY,
                           std::uint8_t angle,
                           std::uint8_t destination,
                           std::uint8_t targetPosition,
                           std::uint16_t targetId,
                           std::uint8_t skillSerial,
                           Season52Crypto& crypto,
                           std::vector<std::uint8_t>& wirePacket) {
    const std::vector<std::uint8_t> canonical{
        0xC1, 0x0D, 0x1E,
        static_cast<std::uint8_t>((skillId >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(skillId & 0xFFu),
        targetX,
        targetY,
        angle,
        destination,
        targetPosition,
        static_cast<std::uint8_t>((targetId >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(targetId & 0xFFu),
        skillSerial
    };
    return crypto.Encode(canonical.data(), canonical.size(), wirePacket);
}

bool BuildTalkNpcRequest(std::uint16_t npcId,
                         Season52Crypto& crypto,
                         std::vector<std::uint8_t>& wirePacket) {
    const std::vector<std::uint8_t> canonical{
        0xC1, 0x05, 0x30,
        static_cast<std::uint8_t>((npcId >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(npcId & 0xFFu)
    };
    return crypto.Encode(canonical.data(), canonical.size(), wirePacket);
}

bool BuildConsumeItemRequest(std::uint8_t itemSlot,
                             std::uint8_t targetSlot,
                             std::uint8_t fruitUsage,
                             Season52Crypto& crypto,
                             std::vector<std::uint8_t>& wirePacket) {
    const std::vector<std::uint8_t> canonical{
        0xC1, 0x06, 0x26, itemSlot, targetSlot, fruitUsage
    };
    return crypto.Encode(canonical.data(), canonical.size(), wirePacket);
}

bool BuildBuyItemRequest(std::uint8_t itemSlot,
                         Season52Crypto& crypto,
                         std::vector<std::uint8_t>& wirePacket) {
    const std::vector<std::uint8_t> canonical{
        0xC1, 0x04, 0x32, itemSlot
    };
    return crypto.Encode(canonical.data(), canonical.size(), wirePacket);
}

bool BuildSellItemRequest(std::uint8_t itemSlot,
                          Season52Crypto& crypto,
                          std::vector<std::uint8_t>& wirePacket) {
    const std::vector<std::uint8_t> canonical{
        0xC1, 0x04, 0x33, itemSlot
    };
    return crypto.Encode(canonical.data(), canonical.size(), wirePacket);
}

bool BuildRepairItemRequest(std::uint8_t itemSlot,
                            std::uint8_t addGold,
                            Season52Crypto& crypto,
                            std::vector<std::uint8_t>& wirePacket) {
    const std::vector<std::uint8_t> canonical{
        0xC1, 0x05, 0x34, itemSlot, addGold
    };
    return crypto.Encode(canonical.data(), canonical.size(), wirePacket);
}

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
                       std::vector<std::uint8_t>& wirePacket) {
    constexpr std::size_t MapAuthFieldSize = 12;

    auto accountField = FixedField<MapAuthFieldSize>(account);
    const auto characterField = FixedField<MapAuthFieldSize>(character);

    // Louis/Webzen B1:01 applies BUX only to the account field.
    BuxTransform(accountField.data(), accountField.size());

    std::vector<std::uint8_t> plain;
    plain.reserve(69);

    plain.push_back(0xC1);
    plain.push_back(69);
    plain.push_back(0xB1);
    plain.push_back(0x01);

    plain.insert(plain.end(), accountField.begin(), accountField.end());
    plain.insert(plain.end(), characterField.begin(), characterField.end());

    WriteU32LE(plain, authCode1);
    WriteU32LE(plain, authCode2);
    WriteU32LE(plain, authCode3);
    WriteU32LE(plain, authCode4);
    WriteU32LE(plain, tickCount);

    for (std::size_t i = 0; i < version.size(); ++i) {
        plain.push_back(static_cast<std::uint8_t>(version[i] - (i + 1u)));
    }

    plain.insert(plain.end(), serial.begin(), serial.end());

    if (plain.size() != 69) {
        return false;
    }

    return crypto.Encode(plain.data(), plain.size(), wirePacket);
}

bool BuildLoginRequest(std::string_view account,
                       std::string_view password,
                       std::uint32_t tickCount,
                       const std::array<std::uint8_t, ProtocolVersionSize>& version,
                       const std::array<std::uint8_t, ProtocolSerialSize>& serial,
                       Season52Crypto& crypto,
                       std::vector<std::uint8_t>& wirePacket) {
    auto accountField = FixedField<AccountSize>(account);
    auto passwordField = FixedField<PasswordSize>(password);

    BuxTransform(accountField.data(), accountField.size());
    BuxTransform(passwordField.data(), passwordField.size());

    // Canonical pre-encryption packet:
    // C1 size F1 01 account[10] password[20] tick[4] version[5] serial[16]
    std::vector<std::uint8_t> plain;
    plain.reserve(4u + AccountSize + PasswordSize + 4u
                  + ProtocolVersionSize + ProtocolSerialSize);

    plain.push_back(0xC1);
    plain.push_back(0); // patched below
    plain.push_back(0xF1);
    plain.push_back(0x01);

    plain.insert(plain.end(), accountField.begin(), accountField.end());
    plain.insert(plain.end(), passwordField.begin(), passwordField.end());
    WriteU32LE(plain, tickCount);

    // Louis/Webzen sends each protocol version byte decremented by its
    // one-based position.
    for (std::size_t i = 0; i < version.size(); ++i) {
        plain.push_back(static_cast<std::uint8_t>(version[i] - (i + 1u)));
    }

    plain.insert(plain.end(), serial.begin(), serial.end());

    if (plain.size() >= 256u) {
        return false;
    }
    plain[1] = static_cast<std::uint8_t>(plain.size());

    return crypto.Encode(plain.data(), plain.size(), wirePacket);
}

} // namespace mu::net::s52
